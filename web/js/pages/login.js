(function () {
    'use strict';
    const form = document.getElementById('loginForm');
    const usernameInput = document.getElementById('username');
    const passwordInput = document.getElementById('password');
    const rememberCheckbox = document.getElementById('remember');
    const btnLogin = document.getElementById('btnLogin');
    const messageEl = document.getElementById('message');
    async function checkExistingAuth() {
        const token = Auth.getToken();
        if (token) {
            showMessage('message', 'Verificando sessão...', 'info');
            const isValid = await Auth.verify();
            if (isValid) {
                showMessage('message', 'Você já está autenticado. Redirecionando...', 'success');
                setTimeout(() => {
                    window.location.href = '/browseroso';
                }, 1000);
            }
            else {
                hideMessageEl();
            }
        }
    }
    function hideMessageEl() {
        if (messageEl) {
            messageEl.className = 'login-message';
        }
    }
    async function handleLogin(e) {
        e.preventDefault();
        const username = usernameInput.value.trim();
        const password = passwordInput.value;
        const remember = rememberCheckbox.checked;
        if (!username) {
            showMessage('message', 'Por favor, digite seu usuário.', 'error');
            usernameInput.focus();
            return;
        }
        btnLogin.disabled = true;
        const originalText = btnLogin.textContent;
        btnLogin.innerHTML = '<span class="spinner"></span> Entrando...';
        try {
            const result = await Auth.login(username, password, remember);
            if (result.success) {
                showMessage('message', 'Login realizado com sucesso! Redirecionando...', 'success');
                const params = new URLSearchParams(window.location.search);
                const redirect = params.get('redirect') || '/browseroso';
                setTimeout(() => {
                    window.location.href = redirect;
                }, 1000);
            }
            else {
                showMessage('message', result.message || 'Erro ao fazer login.', 'error');
                btnLogin.disabled = false;
                btnLogin.textContent = originalText;
            }
        }
        catch (error) {
            showMessage('message', 'Erro de conexão. Tente novamente.', 'error');
            btnLogin.disabled = false;
            btnLogin.textContent = originalText;
        }
    }
    function init() {
        if (form) {
            form.addEventListener('submit', handleLogin);
        }
        if (usernameInput) {
            usernameInput.focus();
        }
        checkExistingAuth();
    }
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    }
    else {
        init();
    }
})();
//# sourceMappingURL=login.js.map