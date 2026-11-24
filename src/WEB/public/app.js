'use strict';

const API_BASE = '';
const REQUEST_TIMEOUT_MS = 8000;
const LOG_MAX = 15;

const ERR_KO = {
  INVALID_PARAMS: '입력값을 확인하세요.',
  METHOD_NOT_ALLOWED: '허용되지 않은 요청 방식입니다.',
  MISSING_USER_ID: 'user_id가 필요합니다.',
  USER_NOT_FOUND: '존재하지 않는 사용자입니다.',
  ROOM_NOT_FOUND: '방을 찾을 수 없습니다.',
  WRONG_PASSWORD: '비밀번호가 틀렸습니다.',
  USER_ALREADY_EXISTS: '이미 사용 중인 아이디입니다.',
  MDN_ALREADY_EXISTS: '이미 등록된 전화번호입니다.',
  ROOM_ALREADY_EXISTS: '같은 이름의 방이 이미 있습니다.',
  NOT_FRIEND: '친구가 아닌 사용자가 포함되어 있습니다.',
  SERVER_ERROR: '서버 오류가 발생했습니다.',
  INVALID_SESSION: '로그인이 만료되었습니다. 다시 로그인하세요.',
  NOT_MEMBER: '이 방의 멤버가 아닙니다. 먼저 입장하세요.',
  NOT_FOUND: '서버에 없는 요청입니다.',
  TOO_LARGE: '요청이 너무 큽니다.',
  TIMEOUT: '서버 응답이 없습니다. (아직 서버에 없는 API일 수 있습니다)',
  NETWORK_ERROR: '서버에 연결할 수 없습니다.',
  BAD_RESPONSE: '서버 응답을 해석할 수 없습니다.',
};

const state = {
  me: null,
  session: null,
  friends: [],
  rooms: [],
  current: null,
  roomInfo: {},
  joined: new Set(),
  messages: {},
  ws: null,
  wsRetry: 0,
  wsTimer: null,
};

const $ = (sel) => document.querySelector(sel);

function h(tag, attrs, ...children) {
  const el = document.createElement(tag);
  Object.entries(attrs || {}).forEach(([k, v]) => {
    if (v === false || v === null || v === undefined) return;
    if (k === 'class') el.className = v;
    else if (k.startsWith('on')) el.addEventListener(k.slice(2), v);
    else if (v === true) el.setAttribute(k, '');
    else el.setAttribute(k, v);
  });
  children.flat().forEach((c) => {
    if (c === null || c === undefined || c === false) return;
    el.appendChild(typeof c === 'string' || typeof c === 'number' ? document.createTextNode(String(c)) : c);
  });
  return el;
}

function show(el, on) { el.hidden = !on; }
function initial(s) { return (s || '?').charAt(0).toUpperCase(); }
function hasQuote(...vals) { return vals.some((v) => typeof v === 'string' && /["\\]/.test(v)); }

let toastTimer = null;
function toast(ok, code, text) {
  const t = $('#toast');
  t.className = 'toast ' + (ok ? 'ok' : 'err');
  $('#toast-code').textContent = code || '';
  $('#toast-text').textContent = text || '';
  show(t, true);
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => show(t, false), 6000);
}
function toastRes(res, okText) {
  if (res.success) { if (okText) toast(true, 'SUCCESS', okText); }
  else toast(false, res.code, ERR_KO[res.code] || res.message || '요청 실패');
}

function addLog(line, body, status, res) {
  const ok = status >= 200 && status < 300;
  const entry = h('div', { class: 'log-entry' },
    h('div', { class: 'log-line' }, h('span', { class: 'badge ' + (ok ? 'ok' : 'err') }, status || '---'), line),
    body ? h('pre', { class: 'req' }, body) : null,
    h('pre', {}, typeof res === 'string' ? res : JSON.stringify(res, null, 2)));
  const list = $('#log-list');
  list.prepend(entry);
  while (list.children.length > LOG_MAX) list.lastChild.remove();
}

async function api(method, path, body) {
  const ctrl = new AbortController();
  const timer = setTimeout(() => ctrl.abort(), REQUEST_TIMEOUT_MS);
  const opts = { method, signal: ctrl.signal, headers: {} };
  const bodyText = body ? JSON.stringify(body) : '';
  if (body) {
    opts.headers['Content-Type'] = 'application/json';
    opts.body = bodyText;
  }

  let status = 0;
  let res;
  try {
    const r = await fetch(API_BASE + path, opts);
    status = r.status;
    const text = await r.text();
    try { res = JSON.parse(text); }
    catch (_) { res = { success: false, code: 'BAD_RESPONSE', message: text.slice(0, 200), data: null }; }
  } catch (e) {
    res = { success: false, code: e.name === 'AbortError' ? 'TIMEOUT' : 'NETWORK_ERROR', message: String(e.message || e), data: null };
  } finally {
    clearTimeout(timer);
  }

  const mask = (s) => s.replace(/"(password|session_id)":"[^"]*"/g, '"$1":"***"').replace(/session_id=[^&]*/g, 'session_id=***');
  addLog(method + ' ' + mask(path), mask(bodyText), status, res);

  if (res.code === 'INVALID_SESSION' && state.me) {
    logout();
    toast(false, res.code, ERR_KO.INVALID_SESSION);
  }
  return res;
}

function setAuthTab(tab) {
  const login = tab === 'login';
  $('#tab-login').classList.toggle('on', login);
  $('#tab-signup').classList.toggle('on', !login);
  $('#tab-login').setAttribute('aria-selected', String(login));
  $('#tab-signup').setAttribute('aria-selected', String(!login));
  show($('#login-form'), login);
  show($('#signup-form'), !login);
}

async function onLogin(e) {
  e.preventDefault();
  const f = e.target;
  const user_id = f.user_id.value.trim();
  const password = f.password.value;
  if (hasQuote(user_id, password)) return toast(false, 'INVALID_PARAMS', '큰따옴표(")와 역슬래시(\\)는 쓸 수 없습니다.');

  const res = await api('POST', '/login', { user_id, password });
  if (!res.success) return toastRes(res);

  const data = res.data || {};
  state.me = { user_id, name: data.name || user_id };
  state.session = data.session_id || null;
  sessionStorage.setItem('zt_me', JSON.stringify({ me: state.me, session: state.session }));
  f.password.value = '';
  enterApp();
}

async function onSignup(e) {
  e.preventDefault();
  const f = e.target;
  const body = { user_id: f.user_id.value.trim(), name: f.name.value.trim(), password: f.password.value, mdn: f.mdn.value.trim() };
  if (hasQuote(body.user_id, body.name, body.password, body.mdn)) return toast(false, 'INVALID_PARAMS', '큰따옴표(")와 역슬래시(\\)는 쓸 수 없습니다.');

  const res = await api('POST', '/join', body);
  toastRes(res, '가입 완료! 이제 로그인하세요.');
  if (res.success) {
    f.reset();
    $('#login-form').user_id.value = body.user_id;
    setAuthTab('login');
  }
}

function logout() {
  sessionStorage.removeItem('zt_me');
  closeWs();
  Object.assign(state, { me: null, session: null, friends: [], rooms: [], current: null, roomInfo: {}, messages: {} });
  state.joined = new Set();
  show($('#auth'), true);
  ['#sidebar', '#panel', '#user-box'].forEach((s) => show($(s), false));
}

async function enterApp() {
  show($('#auth'), false);
  ['#sidebar', '#panel', '#user-box'].forEach((s) => show($(s), true));
  $('#user-name').textContent = state.me.name;
  $('#user-id').textContent = '@' + state.me.user_id;
  $('#user-avatar').textContent = initial(state.me.name);
  setView('home');
  await Promise.all([loadFriends(), loadRooms()]);
  connectWs();
}

async function loadFriends() {
  const res = await api('GET', '/friends?user_id=' + encodeURIComponent(state.me.user_id));
  state.friends = res.success && Array.isArray(res.data) ? res.data : [];
  renderFriends();
}

async function loadRooms() {
  const res = await api('GET', '/rooms');
  if (!res.success) toastRes(res);
  state.rooms = res.success && Array.isArray(res.data) ? res.data : [];
  renderRooms();
}

function isOwner(room) { return room && room.user_id === state.me.user_id; }
function canEnter(roomId) {
  const room = state.rooms.find((r) => r.room_id === roomId);
  const info = state.roomInfo[roomId];
  const member = info && Array.isArray(info.members) && info.members.includes(state.me.user_id);
  return isOwner(room) || member || state.joined.has(roomId);
}

function renderRooms() {
  const list = $('#room-list');
  list.replaceChildren(...state.rooms.map((r) => h('button', {
    class: 'item' + (state.current === r.room_id ? ' on' : ''), type: 'button', onclick: () => openRoom(r.room_id),
  },
  h('span', { class: 'type' }, r.room_type),
  h('span', { class: 'txt' }, h('span', { class: 'name' }, r.room_id), h('span', { class: 'muted' }, '방장 ' + r.user_id)),
  h('span', { class: 'pill ' + (canEnter(r.room_id) ? 'pill-on' : 'pill-off') }, canEnter(r.room_id) ? '입장 가능' : '잠김'))));
  if (!state.rooms.length) list.replaceChildren(h('p', { class: 'muted', style: 'padding:8px 12px' }, '방이 없습니다.'));
  $('#room-count').textContent = state.rooms.length + '개';
}

function renderFriends() {
  const list = $('#friend-list');
  list.replaceChildren(...state.friends.map((f) => h('div', { class: 'friend' },
    h('div', { class: 'avatar' }, initial(f.name)),
    h('div', {}, h('div', {}, f.name + ' ', h('span', { class: 'muted' }, '@' + f.user_id)), h('div', { class: 'muted' }, f.mdn)))));
  if (!state.friends.length) list.replaceChildren(h('p', { class: 'muted', style: 'padding:8px 12px' }, '아직 친구가 없습니다.'));
  $('#friend-count').textContent = state.friends.length + '명';
}

function friendChecks(container, name, excludeIds) {
  const opts = state.friends.filter((f) => !(excludeIds || []).includes(f.user_id));
  container.replaceChildren(...opts.map((f) => h('label', { class: 'chk' },
    h('input', { type: 'checkbox', name, value: f.user_id }), f.name + ' ', h('span', { class: 'muted' }, '@' + f.user_id))));
  if (!opts.length) container.replaceChildren(h('p', { class: 'muted' }, '선택할 수 있는 친구가 없습니다.'));
}
function checkedValues(form, name) {
  return Array.from(form.querySelectorAll('input[name="' + name + '"]:checked')).map((i) => i.value);
}

function setView(view) {
  show($('#view-home'), view === 'home');
  show($('#view-create'), view === 'create');
  show($('#view-room'), view === 'room');
  if (view !== 'room') state.current = null;
  renderRooms();
}

function openCreate() {
  const f = $('#view-create');
  f.reset();
  friendChecks($('#create-options'), 'member');
  updateRoomIdPreview();
  setView('create');
}
function updateRoomIdPreview() {
  $('#room-id-preview').textContent = ($('#view-create').room_id.value.trim() || '<room_id>') + '_' + state.me.user_id;
}

async function onCreate(e) {
  e.preventDefault();
  const f = e.target;
  const members = checkedValues(f, 'member');
  const room_type = f.room_type.value;
  const body = { room_id: f.room_id.value.trim(), password: f.password.value, user_id: state.me.user_id, room_type, members: members.join(',') };
  if (hasQuote(body.room_id, body.password)) return toast(false, 'INVALID_PARAMS', '큰따옴표(")와 역슬래시(\\)는 쓸 수 없습니다.');
  if (room_type === '1:1' && members.length !== 1) return toast(false, 'INVALID_PARAMS', '1:1 방은 친구를 정확히 1명 골라야 합니다.');
  if (room_type === '1:N' && members.length < 1) return toast(false, 'INVALID_PARAMS', '친구를 1명 이상 고르세요.');

  const res = await api('POST', '/room', body);
  toastRes(res, '방이 만들어졌습니다.');
  if (!res.success) return;
  const roomId = (res.data && res.data.room_id) || body.room_id + '_' + state.me.user_id;
  state.joined.add(roomId);
  await loadRooms();
  openRoom(roomId);
}

async function onSearch(e) {
  e.preventDefault();
  const q = e.target.q.value.trim();
  const res = await api('GET', '/room?id=' + encodeURIComponent(q));
  if (!res.success) return toastRes(res);
  if (!state.rooms.some((r) => r.room_id === q)) await loadRooms();
  openRoom(q, res.data);
}

function parseMembers(v) {
  if (Array.isArray(v)) return v;
  if (typeof v === 'string') return v.split(',').map((s) => s.trim()).filter(Boolean);
  return null;
}

async function openRoom(roomId, searchData) {
  state.current = roomId;
  const room = state.rooms.find((r) => r.room_id === roomId) || { room_id: roomId, user_id: '?', room_type: '?' };
  $('#room-title').textContent = roomId;
  $('#room-meta').textContent = room.room_type + ' · 방장 ' + room.user_id + ' · 채널 room:' + roomId;
  setView('room');
  state.current = roomId;
  renderRooms();

  let data = searchData;
  if (!data) {
    const res = await api('GET', '/room?id=' + encodeURIComponent(roomId));
    data = res.success ? res.data : null;
  }
  const members = data ? parseMembers(data.members) : null;
  if (members) state.roomInfo[roomId] = { members };
  renderRoom();
  if (canEnter(roomId)) loadHistory(roomId);
}

function renderRoom() {
  const roomId = state.current;
  if (!roomId) return;
  const room = state.rooms.find((r) => r.room_id === roomId);
  const open = canEnter(roomId);
  show($('#join-gate'), !open);
  show($('#room-body'), open);
  if (!open) return;

  const info = state.roomInfo[roomId];
  const memberIds = info ? info.members : null;
  const ul = $('#members');
  if (memberIds) {
    const all = [room ? room.user_id : null].concat(memberIds).filter((v, i, a) => v && a.indexOf(v) === i);
    ul.replaceChildren(...all.map((id) => h('li', { class: 'row' },
      h('span', { class: 'avatar', style: 'width:28px;height:28px;font-size:12px' }, initial(id)), id,
      h('span', { class: 'muted' }, [room && id === room.user_id ? '방장' : '', id === state.me.user_id ? '나' : ''].filter(Boolean).join(' · ')))));
    friendChecks($('#invite-options'), 'invite', all);
  } else {
    ul.replaceChildren(h('li', { class: 'muted' }, '멤버 조회 API가 아직 없습니다.'));
    friendChecks($('#invite-options'), 'invite', room ? [room.user_id] : []);
  }
  renderMessages();
}

async function onJoin(e) {
  e.preventDefault();
  const f = e.target;
  const body = { room_id: state.current, password: f.password.value };
  if (state.session) body.session_id = state.session;
  const res = await api('POST', '/room/join', body);
  f.reset();
  toastRes(res, '입장했습니다.');
  if (!res.success) return;
  state.joined.add(state.current);
  wsSend({ type: 'subscribe', room_id: state.current });
  openRoom(state.current);
}

async function onInvite(e) {
  e.preventDefault();
  const members = checkedValues(e.target, 'invite');
  if (!members.length) return toast(false, 'INVALID_PARAMS', '초대할 친구를 1명 이상 고르세요.');
  const res = await api('POST', '/room/invite', { room_id: state.current, user_id: state.me.user_id, members: members.join(',') });
  toastRes(res, members.join(', ') + ' 님을 초대했습니다.');
  if (!res.success) return;
  const info = state.roomInfo[state.current];
  if (info) info.members = info.members.concat(members.filter((m) => !info.members.includes(m)));
  renderRoom();
}

function pushMessage(roomId, msg) {
  const list = state.messages[roomId] || (state.messages[roomId] = []);
  if (msg.id && list.some((m) => m.id === msg.id)) return;
  list.push(msg);
  if (roomId === state.current) renderMessages();
}

function renderMessages() {
  const ol = $('#messages');
  const list = state.messages[state.current] || [];
  ol.replaceChildren(...list.map((m) => {
    const cls = m.system ? 'msg system' : (m.from === state.me.user_id ? 'msg mine' : 'msg');
    const time = m.ts ? new Date(m.ts * 1000).toLocaleTimeString('ko-KR', { hour: '2-digit', minute: '2-digit' }) : '';
    return h('li', { class: cls },
      m.system ? null : h('span', { class: 'meta' }, (m.from === state.me.user_id ? '나' : m.from) + (time ? ' · ' + time : '')),
      h('span', { class: 'bubble' }, m.text));
  }));
  if (!list.length) ol.replaceChildren(h('li', { class: 'msg system' }, h('span', { class: 'bubble' }, '아직 메시지가 없습니다.')));
  ol.scrollTop = ol.scrollHeight;
}

async function loadHistory(roomId) {
  if (!state.session) return;
  const res = await api('GET', '/room/messages?room_id=' + encodeURIComponent(roomId) + '&session_id=' + encodeURIComponent(state.session));
  if (!res.success || !Array.isArray(res.data)) return;
  state.messages[roomId] = [];
  res.data.forEach((m) => pushMessage(roomId, { id: m.id, from: m.from, text: m.text, ts: m.ts }));
  renderMessages();
}

async function onSend(e) {
  e.preventDefault();
  const f = e.target;
  const text = f.text.value.trim();
  if (!text) return;
  if (!state.session) return toast(false, 'INVALID_SESSION', ERR_KO.INVALID_SESSION);
  const res = await api('POST', '/room/message', { room_id: state.current, session_id: state.session, text });
  if (!res.success) return toastRes(res);
  f.reset();
  // With WebSocket up, the published message comes back to us.
  if (!wsOpen()) pushMessage(state.current, { id: res.data && res.data.id, from: state.me.user_id, text, ts: Math.floor(Date.now() / 1000) });
}

function setWsStatus(kind, text) {
  const el = $('#ws-status');
  el.className = 'pill ' + { on: 'pill-on', wait: 'pill-wait', off: 'pill-off' }[kind];
  el.textContent = text;
}
function wsOpen() { return state.ws && state.ws.readyState === WebSocket.OPEN; }
function wsSend(obj) { if (wsOpen()) state.ws.send(JSON.stringify(obj)); }

function connectWs() {
  if (!state.me) return;
  const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const q = state.session ? 'session=' + encodeURIComponent(state.session) : 'user_id=' + encodeURIComponent(state.me.user_id);
  const ws = new WebSocket(proto + '//' + location.host + '/ws?' + q);
  state.ws = ws;
  setWsStatus('wait', '실시간 연결 중');

  ws.onopen = () => {
    state.wsRetry = 0;
    setWsStatus('on', '실시간 연결됨');
    addLog('WS /ws (open)', '', 101, '연결됨');
  };
  ws.onmessage = (ev) => {
    let m;
    try { m = JSON.parse(ev.data); } catch (_) { return; }
    addLog('WS ← ' + (m.type || '?'), '', 200, m);
    if (m.type === 'message' && m.room_id) {
      pushMessage(m.room_id, { id: m.id, from: m.from, text: m.text, ts: m.ts });
    } else if (m.type === 'invite' && m.room_id) {
      toast(true, 'INVITE', (m.from || '누군가') + ' 님이 ' + m.room_id + ' 방에 초대했습니다.');
      state.joined.add(m.room_id);
      loadRooms();
    }
  };
  ws.onclose = () => {
    if (state.ws !== ws) return;
    state.ws = null;
    if (!state.me) return setWsStatus('off', '실시간 꺼짐');
    state.wsRetry += 1;
    const delay = Math.min(30000, 1000 * 2 ** Math.min(state.wsRetry, 5));
    setWsStatus(state.wsRetry > 3 ? 'off' : 'wait', state.wsRetry > 3 ? '실시간 미지원 (재시도 중)' : '재연결 중');
    state.wsTimer = setTimeout(connectWs, delay);
  };
}

function closeWs() {
  clearTimeout(state.wsTimer);
  const ws = state.ws;
  state.ws = null;
  if (ws) ws.close();
  setWsStatus('off', '실시간 꺼짐');
}

function init() {
  $('#tab-login').addEventListener('click', () => setAuthTab('login'));
  $('#tab-signup').addEventListener('click', () => setAuthTab('signup'));
  $('#login-form').addEventListener('submit', onLogin);
  $('#signup-form').addEventListener('submit', onSignup);
  $('#logout').addEventListener('click', logout);
  $('#toast-close').addEventListener('click', () => show($('#toast'), false));
  $('#to-create').addEventListener('click', openCreate);
  $('#create-cancel').addEventListener('click', () => setView('home'));
  $('#view-create').addEventListener('submit', onCreate);
  $('#view-create').room_id.addEventListener('input', updateRoomIdPreview);
  $('#search-form').addEventListener('submit', onSearch);
  $('#refresh-rooms').addEventListener('click', loadRooms);
  $('#room-close').addEventListener('click', () => setView('home'));
  $('#join-gate').addEventListener('submit', onJoin);
  $('#invite-form').addEventListener('submit', onInvite);
  $('#send-form').addEventListener('submit', onSend);
  $('#log-clear').addEventListener('click', () => $('#log-list').replaceChildren());

  const saved = sessionStorage.getItem('zt_me');
  if (saved) {
    try {
      const s = JSON.parse(saved);
      state.me = s.me;
      state.session = s.session;
      enterApp();
    } catch (_) { sessionStorage.removeItem('zt_me'); }
  }
}

init();
