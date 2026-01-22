class Auth {
    static getToken() {
        return localStorage.getItem(this.TOKEN_KEY) ||
            sessionStorage.getItem(this.TOKEN_KEY);
    }
    static setToken(token, remember = false) {
        if (remember) {
            localStorage.setItem(this.TOKEN_KEY, token);
            localStorage.setItem(this.TOKEN_STORAGE_KEY, 'local');
        }
        else {
            sessionStorage.setItem(this.TOKEN_KEY, token);
            sessionStorage.setItem(this.TOKEN_STORAGE_KEY, 'session');
        }
    }
    static removeToken() {
        localStorage.removeItem(this.TOKEN_KEY);
        localStorage.removeItem(this.TOKEN_STORAGE_KEY);
        sessionStorage.removeItem(this.TOKEN_KEY);
        sessionStorage.removeItem(this.TOKEN_STORAGE_KEY);
    }
    static isAuthenticated() {
        const token = this.getToken();
        if (!token)
            return false;
        try {
            const payload = this.decodeToken(token);
            if (payload && payload.exp) {
                return Date.now() < payload.exp * 1000;
            }
        }
        catch {
            return false;
        }
        return true;
    }
    static decodeToken(token) {
        try {
            const parts = token.split('.');
            if (parts.length !== 3)
                return null;
            const payload = parts[1];
            const decoded = atob(payload.replace(/-/g, '+').replace(/_/g, '/'));
            return JSON.parse(decoded);
        }
        catch {
            return null;
        }
    }
    static async login(username, password, remember = false) {
        try {
            const response = await fetch('/api/auth/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ username, password })
            });
            const data = await response.json();
            if (response.ok && data.success && data.token) {
                this.setToken(data.token, remember);
            }
            return data;
        }
        catch (error) {
            return {
                success: false,
                message: 'Erro de conexão. Tente novamente.'
            };
        }
    }
    static logout() {
        this.removeToken();
        window.location.href = '/login';
    }
    static async verify() {
        const token = this.getToken();
        if (!token)
            return false;
        try {
            const response = await fetch('/api/auth/verify', {
                headers: {
                    'Authorization': `Bearer ${token}`
                }
            });
            if (!response.ok) {
                this.removeToken();
                return false;
            }
            return true;
        }
        catch {
            return false;
        }
    }
    static async refresh() {
        const token = this.getToken();
        if (!token)
            return false;
        try {
            const response = await fetch('/api/auth/refresh', {
                method: 'POST',
                headers: {
                    'Authorization': `Bearer ${token}`
                }
            });
            if (response.ok) {
                const data = await response.json();
                if (data.success && data.token) {
                    const storage = localStorage.getItem(this.TOKEN_STORAGE_KEY);
                    this.setToken(data.token, storage === 'local');
                    return true;
                }
            }
            return false;
        }
        catch {
            return false;
        }
    }
    static async authFetch(url, options = {}) {
        const token = this.getToken();
        const headers = new Headers(options.headers);
        if (token) {
            headers.set('Authorization', `Bearer ${token}`);
        }
        const response = await fetch(url, {
            ...options,
            headers
        });
        if (response.status === 401) {
            this.removeToken();
            window.location.href = '/login';
            throw new Error('Session expired');
        }
        return response;
    }
    static requireAuth() {
        if (!this.isAuthenticated()) {
            window.location.href = '/login';
        }
    }
}
Auth.TOKEN_KEY = 'jwt_token';
Auth.TOKEN_STORAGE_KEY = 'jwt_storage';
window.Auth = Auth;
//# sourceMappingURL=auth.js.map