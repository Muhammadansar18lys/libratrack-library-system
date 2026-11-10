const $ = (selector) => document.querySelector(selector);
const moneylessDate = new Intl.DateTimeFormat([], { weekday: "short", month: "short", day: "numeric" });
$("#today").textContent = moneylessDate.format(new Date());
let currentBooks = [];
let currentMembers = [];
let searchTimer;
let toastTimer;

async function api(path, options = {}) {
  const response = await fetch(path, {
    ...options,
    headers: { "Content-Type": "application/json", ...(options.headers || {}) }
  });
  const data = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(data.message || `Request failed (${response.status})`);
  return data;
}
function escapeHTML(value) {
  return String(value).replace(/[&<>"']/g, (char) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;", "'": "&#39;"
  })[char]);
}
function showToast(message, isError = false) {
  const toast = $("#toast");
  toast.textContent = message;
  toast.style.background = isError ? "#8c4039" : "#243b2e";
  toast.classList.add("show");
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toast.classList.remove("show"), 3000);
}
function initials(name) {
  return name.trim().split(/\s+/).slice(0, 2).map((part) => part[0]?.toUpperCase() || "").join("");
}

async function loadStats() {
  const stats = await api("/api/stats");
  $("#statTitles").textContent = stats.titles;
  $("#statLoans").textContent = stats.activeLoans;
  $("#statMembers").textContent = stats.members;
  $("#statOverdue").textContent = stats.overdue;
  $("#loanNavCount").textContent = stats.activeLoans;
}
async function loadBooks(query = "") {
  const data = await api(`/api/books${query ? `?q=${encodeURIComponent(query)}` : ""}`);
  currentBooks = data.books;
  renderBooks(currentBooks);
}
function renderBooks(books) {
  $("#bookCount").textContent = `${books.length} ${books.length === 1 ? "book" : "books"}`;
  $("#bookEmpty").classList.toggle("hidden", books.length > 0);
  $("#bookRows").innerHTML = books.map((book) => {
    const available = Number(book.copiesAvailable);
    const state = available > 0 ? `${available} of ${book.copiesTotal} available` : "All checked out";
    const coverLetter = escapeHTML(book.title.trim().charAt(0).toUpperCase());
    return `<tr><td><div class="book-cell"><span class="book-cover">${coverLetter}</span><span class="book-title"><strong>${escapeHTML(book.title)}</strong><small>${book.publishedYear || "Year not listed"}</small></span></div></td>
      <td>${escapeHTML(book.author)}</td><td>${escapeHTML(book.isbn)}</td>
      <td><span class="availability ${available === 0 ? "none" : ""}">${state}</span></td>
      <td><div class="row-actions"><button class="small-button" data-queue="${book.id}">Queue</button><button class="small-button" data-borrow="${book.id}" ${$("#borrowMember").value ? "" : "disabled"}>${available > 0 ? "Borrow" : "Reserve"}</button></div></td></tr>`;
  }).join("");
}
async function loadMembers() {
  const data = await api("/api/members");
  currentMembers = data.members;
  $("#borrowMember").innerHTML = '<option value="">Select a member</option>' + currentMembers.map((member) =>
    `<option value="${member.id}">${escapeHTML(member.fullName)} · ${escapeHTML(member.code)}</option>`).join("");
  $("#memberList").innerHTML = currentMembers.slice(0, 6).map((member) =>
    `<div class="member-row"><span class="member-initials">${escapeHTML(initials(member.fullName))}</span><div><strong>${escapeHTML(member.fullName)}</strong><small>${escapeHTML(member.code)} · ${escapeHTML(member.email)}</small></div></div>`).join("") || '<div class="empty">No members yet. Add the first member.</div>';
  renderBooks(currentBooks);
}
async function loadLoans() {
  const data = await api("/api/loans");
  $("#loanCount").textContent = data.loans.length;
  $("#loanNavCount").textContent = data.loans.length;
  $("#loanList").innerHTML = data.loans.slice(0, 7).map((loan) =>
    `<div class="loan-row"><div class="loan-person"><strong>${escapeHTML(loan.title)}</strong><small>${escapeHTML(loan.memberName)} · due ${escapeHTML(loan.dueDate)}</small></div><span class="loan-due ${loan.daysLate > 0 ? "overdue" : ""}">${loan.daysLate > 0 ? `${loan.daysLate}d overdue` : "On loan"}</span><button class="small-button" data-return-book="${loan.bookId}" data-return-member="${loan.memberId}">Return</button></div>`).join("") || '<div class="empty">No active loans right now.</div>';
}
async function refreshDashboard() {
  try {
    await Promise.all([loadStats(), loadBooks($("#bookSearch").value.trim()), loadMembers(), loadLoans()]);
  } catch (error) {
    showToast(`Could not connect to the API: ${error.message}`, true);
  }
}

$("#borrowMember").addEventListener("change", () => renderBooks(currentBooks));
$("#bookSearch").addEventListener("input", () => {
  clearTimeout(searchTimer);
  const query = $("#bookSearch").value.trim();
  searchTimer = setTimeout(async () => {
    try {
      await loadBooks(query);
      const suggestions = await api(`/api/books/suggest?q=${encodeURIComponent(query)}`);
      $("#titleSuggestions").innerHTML = suggestions.suggestions.map((title) => `<option value="${escapeHTML(title)}"></option>`).join("");
    } catch (error) { showToast(error.message, true); }
  }, 180);
});

$("#bookRows").addEventListener("click", async (event) => {
  const queueButton = event.target.closest("[data-queue]");
  if (queueButton) {
    const book = currentBooks.find((entry) => Number(entry.id) === Number(queueButton.dataset.queue));
    if (!book) return;
    try {
      const data = await api(`/api/reservations?bookId=${book.id}`);
      $("#queueTitle").textContent = `Waiting list · ${book.title}`;
      $("#queueList").innerHTML = data.reservations.map((item, index) => `<li>${index + 1}. ${escapeHTML(item.memberName)} <span>joined ${escapeHTML(item.createdAt)}</span></li>`).join("") || "<li>No one is waiting for this book.</li>";
      $("#queueDialog").showModal();
    } catch (error) { showToast(error.message, true); }
    return;
  }
  const borrowButton = event.target.closest("[data-borrow]");
  if (!borrowButton) return;
  const memberId = Number($("#borrowMember").value);
  if (!memberId) { showToast("Choose a member first.", true); return; }
  borrowButton.disabled = true;
  try {
    const result = await api("/api/loans/borrow", { method: "POST", body: JSON.stringify({ memberId, bookId: Number(borrowButton.dataset.borrow) }) });
    showToast(result.message);
    await refreshDashboard();
  } catch (error) { showToast(error.message, true); }
});

$("#loanList").addEventListener("click", async (event) => {
  const button = event.target.closest("[data-return-book]");
  if (!button) return;
  button.disabled = true;
  try {
    const result = await api("/api/loans/return", { method: "POST", body: JSON.stringify({ memberId: Number(button.dataset.returnMember), bookId: Number(button.dataset.returnBook) }) });
    showToast(result.message);
    await refreshDashboard();
  } catch (error) { showToast(error.message, true); button.disabled = false; }
});

function openDialog(id) { $(id).showModal(); }
document.querySelectorAll("[data-close]").forEach((button) => button.addEventListener("click", () => $(`#${button.dataset.close}`).close()));
$("#addBookOpen").addEventListener("click", () => openDialog("#bookDialog"));
$("#addMemberOpen").addEventListener("click", () => openDialog("#memberDialog"));
$("#addMemberInline").addEventListener("click", () => openDialog("#memberDialog"));

$("#bookForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  const form = new FormData(event.currentTarget);
  const payload = Object.fromEntries(form.entries());
  payload.year = Number(payload.year || 0);
  payload.copies = Number(payload.copies);
  try {
    await api("/api/books", { method: "POST", body: JSON.stringify(payload) });
    $("#bookDialog").close(); event.currentTarget.reset();
    await Promise.all([loadBooks($("#bookSearch").value.trim()), loadStats()]);
    showToast("Book added to the catalog.");
  } catch (error) { showToast(error.message, true); }
});
$("#memberForm").addEventListener("submit", async (event) => {
  event.preventDefault();
  const payload = Object.fromEntries(new FormData(event.currentTarget).entries());
  try {
    const data = await api("/api/members", { method: "POST", body: JSON.stringify(payload) });
    $("#memberDialog").close(); event.currentTarget.reset();
    await Promise.all([loadMembers(), loadStats()]);
    showToast(`Member registered: ${data.member.code}`);
  } catch (error) { showToast(error.message, true); }
});

refreshDashboard();
