/**
 * Common utilities and helper functions
 * Funções utilitárias compartilhadas entre todas as páginas
 */

/**
 * Escape HTML to prevent XSS
 */
function escapeHtml(text: string | null | undefined): string {
    if (text === null || text === undefined) return '';
    const div = document.createElement('div');
    div.textContent = String(text);
    return div.innerHTML;
}

/**
 * Format date to locale string
 */
function formatDate(date: Date | string): string {
    const d = typeof date === 'string' ? new Date(date) : date;
    return d.toLocaleDateString('pt-BR', {
        day: '2-digit',
        month: '2-digit',
        year: 'numeric',
        hour: '2-digit',
        minute: '2-digit'
    });
}

/**
 * Show a message element with specified type
 */
function showMessage(
    elementId: string,
    message: string,
    type: 'success' | 'error' | 'warning' | 'info'
): void {
    const element = document.getElementById(elementId);
    if (!element) return;

    element.textContent = message;
    element.className = `login-message login-message--visible login-message--${type}`;
}

/**
 * Hide a message element
 */
function hideMessage(elementId: string): void {
    const element = document.getElementById(elementId);
    if (!element) return;
    element.className = 'login-message';
}

/**
 * Debounce function
 */
function debounce<T extends (...args: unknown[]) => void>(
    func: T,
    wait: number
): (...args: Parameters<T>) => void {
    let timeout: ReturnType<typeof setTimeout> | null = null;

    return function (this: unknown, ...args: Parameters<T>) {
        if (timeout) clearTimeout(timeout);
        timeout = setTimeout(() => func.apply(this, args), wait);
    };
}

/**
 * Query selector shorthand
 */
function $(selector: string): HTMLElement | null {
    return document.querySelector(selector);
}

/**
 * Query selector all shorthand
 */
function $$(selector: string): NodeListOf<HTMLElement> {
    return document.querySelectorAll(selector);
}

/**
 * Add event listener shorthand
 */
function on(
    element: HTMLElement | string | null,
    event: string,
    handler: EventListener
): void {
    const el = typeof element === 'string' ? $(element) : element;
    if (el) el.addEventListener(event, handler);
}

// Export for use in other modules (when using ES modules)
// For now, these are global functions
(window as unknown as Record<string, unknown>).escapeHtml = escapeHtml;
(window as unknown as Record<string, unknown>).formatDate = formatDate;
(window as unknown as Record<string, unknown>).showMessage = showMessage;
(window as unknown as Record<string, unknown>).hideMessage = hideMessage;
(window as unknown as Record<string, unknown>).debounce = debounce;
(window as unknown as Record<string, unknown>).$ = $;
(window as unknown as Record<string, unknown>).$$ = $$;
(window as unknown as Record<string, unknown>).on = on;
