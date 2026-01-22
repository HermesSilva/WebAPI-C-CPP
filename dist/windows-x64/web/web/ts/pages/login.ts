/**
 * Login page functionality
 */

/* global Auth, showMessage */

(function () {
    'use strict';

    const form = document.getElementById('loginForm') as HTMLFormElement;
    const usernameInput = document.getElementById('username') as HTMLInputElement;
    const passwordInput = document.getElementById('password') as HTMLInputElement;
    const rememberCheckbox = document.getElementById('remember') as HTMLInputElement;
    const btnLogin = document.getElementById('btnLogin') as HTMLButtonElement;
    const messageEl = document.getElementById('message');

    // Check if already logged in
    async function checkExistingAuth(): Promise<void> {
        const token = Auth.getToken();
        if (token) {
            showMessage('message', 'Verificando sessão...', 'info');

            const isValid = await Auth.verify();
            if (isValid) {
                showMessage('message', 'Você já está autenticado. Redirecionando...', 'success');
                setTimeout(() => {
                    window.location.href = '/browseroso';
                }, 1000);
            } else {
                hideMessageEl();
            }
        }
    }

    function hideMessageEl(): void {
        if (messageEl) {
            messageEl.className = 'login-message';
        }
    }

    // Handle form submission
    async function handleLogin(e: Event): Promise<void> {
        e.preventDefault();

        const username = usernameInput.value.trim();
        const password = passwordInput.value;
        const remember = rememberCheckbox.checked;

        if (!username) {
            showMessage('message', 'Por favor, digite seu usuário.', 'error');
            usernameInput.focus();
            return;
        }

        // Disable button and show loading
        btnLogin.disabled = true;
        const originalText = btnLogin.textContent;
        btnLogin.innerHTML = '<span class="spinner"></span> Entrando...';

        try {
            const result = await Auth.login(username, password, remember);

            if (result.success) {
                showMessage('message', 'Login realizado com sucesso! Redirecionando...', 'success');

                // Get redirect URL from query params or default to browseroso
                const params = new URLSearchParams(window.location.search);
                const redirect = params.get('redirect') || '/browseroso';

                setTimeout(() => {
                    window.location.href = redirect;
                }, 1000);
            } else {
                showMessage('message', result.message || 'Erro ao fazer login.', 'error');
                btnLogin.disabled = false;
                btnLogin.textContent = originalText;
            }
        } catch (error) {
            showMessage('message', 'Erro de conexão. Tente novamente.', 'error');
            btnLogin.disabled = false;
            btnLogin.textContent = originalText;
        }
    }

    // Initialize
    function init(): void {
        if (form) {
            form.addEventListener('submit', handleLogin);
        }

        // Focus username field
        if (usernameInput) {
            usernameInput.focus();
        }

        // Check existing authentication
        checkExistingAuth();
    }

    // Run when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
