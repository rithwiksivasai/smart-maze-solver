const gridElement = document.querySelector('#mazeGrid');
const slider = document.querySelector('#sizeSlider');
const speedButtons = [...document.querySelectorAll('[data-speed]')];
const algorithmButtons = [...document.querySelectorAll('[data-algorithm]')];
const state = { rows: 21, cols: 31, grid: [], start: [1, 1], end: [19, 29], algorithm: 'A*', speed: 32, busy: false, paused: false, drawing: false, drawValue: true, dragVisited: new Set(), runId: 0 };
const $ = (selector) => document.querySelector(selector);

function makeEmptyGrid(rows, cols) { return Array.from({ length: rows }, () => Array(cols).fill('.')); }
function setStatus(text, active = false) { $('#statusText').textContent = text; $('.status-pulse').classList.toggle('running', active); }
function isEndpoint(r, c) { return (r === state.start[0] && c === state.start[1]) || (r === state.end[0] && c === state.end[1]); }
function cellSize() { return state.cols > 35 ? 9 : state.cols > 29 ? 11 : state.cols > 23 ? 13 : 15; }
function renderGrid() {
  gridElement.style.setProperty('--rows', state.rows);
  gridElement.style.setProperty('--cols', state.cols);
  gridElement.style.setProperty('--cell-size', `${cellSize()}px`);
  const fragment = document.createDocumentFragment();
  for (let r = 0; r < state.rows; r++) for (let c = 0; c < state.cols; c++) {
    const cell = document.createElement('div');
    const endpoint = isEndpoint(r, c);
    cell.className = `maze-cell${state.grid[r][c] === '#' ? ' wall' : ''}${r === state.start[0] && c === state.start[1] ? ' start' : ''}${r === state.end[0] && c === state.end[1] ? ' end' : ''}`;
    cell.dataset.r = r; cell.dataset.c = c;
    cell.setAttribute('role', 'gridcell');
    cell.setAttribute('aria-label', endpoint ? (r === state.start[0] && c === state.start[1] ? 'Start' : 'End') : state.grid[r][c] === '#' ? 'Wall' : 'Open cell');
    fragment.append(cell);
  }
  gridElement.replaceChildren(fragment);
}
function makeMaze() {
  state.grid = makeEmptyGrid(state.rows, state.cols);
  for (let r = 0; r < state.rows; r++) for (let c = 0; c < state.cols; c++) if (r % 2 === 0 || c % 2 === 0) state.grid[r][c] = '#';
  const stack = [[1, 1]], visited = new Set(['1,1']); state.grid[1][1] = '.';
  while (stack.length) {
    const [r, c] = stack[stack.length - 1];
    const choices = [[r - 2, c], [r, c + 2], [r + 2, c], [r, c - 2]].filter(([nr, nc]) => nr > 0 && nr < state.rows - 1 && nc > 0 && nc < state.cols - 1 && !visited.has(`${nr},${nc}`));
    if (!choices.length) { stack.pop(); continue; }
    const [nr, nc] = choices[Math.floor(Math.random() * choices.length)];
    visited.add(`${nr},${nc}`); state.grid[(r + nr) / 2][(c + nc) / 2] = '.'; state.grid[nr][nc] = '.'; stack.push([nr, nc]);
  }
  state.start = [1, 1]; state.end = [state.rows - 2, state.cols - 2];
  state.grid[state.start[0]][state.start[1]] = '.'; state.grid[state.end[0]][state.end[1]] = '.';
  resetMetrics(); renderGrid(); setStatus('READY TO EXPLORE');
}
function resetMetrics() {
  $('#visitedMetric').textContent = '—'; $('#pathMetric').textContent = '—'; $('#timeMetric').textContent = '—'; $('#visitedBar').style.width = '0%';
  $('#resultBadge').textContent = 'AWAITING RUN'; $('#resultBadge').classList.remove('complete');
}
function setBusy(value) { state.busy = value; $('#solveBtn').disabled = value; $('#generateBtn').disabled = value; $('#pauseBtn').disabled = value; }
function clearOverlay() { document.querySelectorAll('.maze-cell.visited,.maze-cell.path').forEach(cell => cell.classList.remove('visited', 'path')); }
function getCell(r, c) { return gridElement.children[r * state.cols + c]; }
function wait(ms) { return new Promise(resolve => setTimeout(resolve, ms)); }
async function animate(points, className, runId) {
  for (let i = 0; i < points.length; i++) {
    if (runId !== state.runId) return false;
    while (state.paused && runId === state.runId) await wait(50);
    if (runId !== state.runId) return false;
    const [r, c] = points[i];
    if (!isEndpoint(r, c)) getCell(r, c)?.classList.add(className);
    if (i % 7 === 0) await wait(state.speed);
  }
  return true;
}
async function solveMaze() {
  if (state.busy) return;
  state.runId++; const runId = state.runId; setBusy(true); clearOverlay(); setStatus('C++ ENGINE IS SEARCHING', true);
  $('#pauseBtn').disabled = true;
  try {
    const response = await fetch('/api/solve', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ grid: state.grid.map(row => row.join('')), start: { r: state.start[0], c: state.start[1] }, goal: { r: state.end[0], c: state.end[1] }, algorithm: state.algorithm }) });
    if (!response.ok) throw new Error('Solver request failed');
    const result = await response.json();
    $('#pauseBtn').disabled = false;
    if (!await animate(result.visited, 'visited', runId)) return;
    if (result.found) await animate(result.path, 'path', runId);
    if (runId !== state.runId) return;
    $('#visitedMetric').textContent = result.visitedCount.toLocaleString();
    $('#pathMetric').textContent = result.pathLength.toLocaleString();
    $('#timeMetric').textContent = result.elapsedMs < 0.01 ? '<0.01' : result.elapsedMs.toFixed(2);
    $('#visitedBar').style.width = `${Math.max(3, Math.min(100, result.visitedCount / (state.rows * state.cols) * 100))}%`;
    $('#resultBadge').textContent = result.found ? `${state.algorithm.toUpperCase()} · PATH FOUND` : 'NO PATH FOUND';
    $('#resultBadge').classList.toggle('complete', result.found);
    setStatus(result.found ? 'PATH FOUND — SEARCH COMPLETE' : 'NO PATH TO GOAL');
  } catch (error) {
    console.error(error); setStatus('BACKEND OFFLINE — START THE C++ SERVER');
  } finally { if (runId === state.runId) setBusy(false); }
}
function stopRun() { state.runId++; state.paused = false; $('#pauseBtn').textContent = 'Ⅱ'; setBusy(false); $('#pauseBtn').disabled = true; setStatus('RUN RESET'); }
function resizeMaze(rows) {
  state.rows = rows; state.cols = Math.min(41, rows + 10);
  state.start = [1, 1]; state.end = [rows - 2, state.cols - 2];
  $('#sizeValue').textContent = `${state.rows} × ${state.cols}`;
  slider.style.background = `linear-gradient(90deg,var(--green) ${((rows - 15) / 16) * 100}%,#3b443b ${((rows - 15) / 16) * 100}%)`;
  makeMaze();
}
function toggleWall(event) {
  const cell = event.target.closest('.maze-cell'); if (!cell) return;
  const r = Number(cell.dataset.r), c = Number(cell.dataset.c);
  if (isEndpoint(r, c)) return;
  const key = `${r},${c}`; if (state.dragVisited.has(key)) return;
  state.dragVisited.add(key); state.grid[r][c] = state.drawValue ? '#' : '.';
  cell.classList.toggle('wall', state.drawValue); cell.classList.remove('visited', 'path');
}
algorithmButtons.forEach(button => button.addEventListener('click', () => {
  if (state.busy) return;
  algorithmButtons.forEach(item => item.setAttribute('aria-pressed', String(item === button)));
  state.algorithm = button.dataset.algorithm;
}));
speedButtons.forEach(button => button.addEventListener('click', () => {
  speedButtons.forEach(item => item.classList.toggle('selected', item === button));
  state.speed = Number(button.dataset.speed); $('#speedValue').textContent = button.textContent;
}));
slider.addEventListener('input', () => resizeMaze(Number(slider.value)));
$('#generateBtn').addEventListener('click', () => { stopRun(); makeMaze(); });
$('#solveBtn').addEventListener('click', solveMaze);
$('#clearBtn').addEventListener('click', () => { stopRun(); state.grid = makeEmptyGrid(state.rows, state.cols); resetMetrics(); renderGrid(); setStatus('WALLS CLEARED'); });
$('#resetBtn').addEventListener('click', () => { stopRun(); clearOverlay(); resetMetrics(); setStatus('MAZE RESET'); });
$('#pauseBtn').addEventListener('click', () => {
  if (!state.busy) return;
  state.paused = !state.paused;
  $('#pauseBtn').textContent = state.paused ? '▶' : 'Ⅱ';
  setStatus(state.paused ? 'ANIMATION PAUSED' : 'C++ ENGINE IS SEARCHING', !state.paused);
});
$('#fullscreenBtn').addEventListener('click', async () => {
  const panel = $('.maze-panel');
  try { if (!document.fullscreenElement) await panel.requestFullscreen(); else await document.exitFullscreen(); } catch { setStatus('FULLSCREEN NOT AVAILABLE'); }
});
gridElement.addEventListener('pointerdown', event => {
  if (state.busy) return;
  const cell = event.target.closest('.maze-cell'); if (!cell) return;
  const r = Number(cell.dataset.r), c = Number(cell.dataset.c); if (isEndpoint(r, c)) return;
  event.preventDefault(); state.drawing = true; state.dragVisited.clear(); state.drawValue = state.grid[r][c] !== '#'; toggleWall(event);
});
gridElement.addEventListener('pointermove', event => { if (state.drawing) toggleWall(event); });
window.addEventListener('pointerup', () => { state.drawing = false; state.dragVisited.clear(); });
slider.style.background = 'linear-gradient(90deg,var(--green) 37%,#3b443b 37%)';
makeMaze();
