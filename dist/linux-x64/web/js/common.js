function escapeHtml(text) {
    if (text === null || text === undefined)
        return '';
    const div = document.createElement('div');
    div.textContent = String(text);
    return div.innerHTML;
}
function formatDate(date) {
    const d = typeof date === 'string' ? new Date(date) : date;
    return d.toLocaleDateString('pt-BR', {
        day: '2-digit',
        month: '2-digit',
        year: 'numeric',
        hour: '2-digit',
        minute: '2-digit'
    });
}
function showMessage(elementId, message, type) {
    const element = document.getElementById(elementId);
    if (!element)
        return;
    element.textContent = message;
    element.className = `login-message login-message--visible login-message--${type}`;
}
function hideMessage(elementId) {
    const element = document.getElementById(elementId);
    if (!element)
        return;
    element.className = 'login-message';
}
function debounce(func, wait) {
    let timeout = null;
    return function (...args) {
        if (timeout)
            clearTimeout(timeout);
        timeout = setTimeout(() => func.apply(this, args), wait);
    };
}
function $(selector) {
    return document.querySelector(selector);
}
function $$(selector) {
    return document.querySelectorAll(selector);
}
function on(element, event, handler) {
    const el = typeof element === 'string' ? $(element) : element;
    if (el)
        el.addEventListener(event, handler);
}
window.escapeHtml = escapeHtml;
window.formatDate = formatDate;
window.showMessage = showMessage;
window.hideMessage = hideMessage;
window.debounce = debounce;
window.$ = $;
window.$$ = $$;
window.on = on;
//# sourceMappingURL=common.js.map