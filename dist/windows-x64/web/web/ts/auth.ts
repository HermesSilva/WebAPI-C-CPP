/**
 * Authentication module
 * Gerenciamento de autenticação JWT
 */

interface AuthResponse {
    success: boolean;
    token?: string;
    message?: string;
    user?: {
        username: string;
    };
}

interface JWTPayload {
    sub: string;
    exp: number;
    iat: number;
}

/**
 * Auth class for managing JWT authentication
 */
class Auth {
    private static TOKEN_KEY = 'jwt_token';
    private static TOKEN_STORAGE_KEY = 'jwt_storage';

    /**
     * Get the stored token
     */
    static getToken(): string | null {
        // Check localStorage first, then sessionStorage
        return localStorage.getItem(this.TOKEN_KEY) ||
            sessionStorage.getItem(this.TOKEN_KEY);
    }

    /**
     * Save token to storage
     */
    static setToken(token: string, remember: boolean = false): void {
        if (remember) {
            localStorage.setItem(this.TOKEN_KEY, token);
            localStorage.setItem(this.TOKEN_STORAGE_KEY, 'local');
        } else {
            sessionStorage.setItem(this.TOKEN_KEY, token);
            sessionStorage.setItem(this.TOKEN_STORAGE_KEY, 'session');
        }
    }

    /**
     * Remove token from storage
     */
    static removeToken(): void {
        localStorage.removeItem(this.TOKEN_KEY);
        localStorage.removeItem(this.TOKEN_STORAGE_KEY);
        sessionStorage.removeItem(this.TOKEN_KEY);
        sessionStorage.removeItem(this.TOKEN_STORAGE_KEY);
    }

    /**
     * Check if user is authenticated (has valid token)
     */
    static isAuthenticated(): boolean {
        const token = this.getToken();
        if (!token) return false;

        // Check if token is expired
        try {
            const payload = this.decodeToken(token);
            if (payload && payload.exp) {
                return Date.now() < payload.exp * 1000;
            }
        } catch {
            return false;
        }

        return true;
    }

    /**
     * Decode JWT token payload
     */
    static decodeToken(token: string): JWTPayload | null {
        try {
            const parts = token.split('.');
            if (parts.length !== 3) return null;

            const payload = parts[1];
            const decoded = atob(payload.replace(/-/g, '+').replace(/_/g, '/'));
            return JSON.parse(decoded);
        } catch {
            return null;
        }
    }

    /**
     * Login with username and password
     */
    static async login(username: string, password: string, remember: boolean = false): Promise<AuthResponse> {
        try {
            const response = await fetch('/api/auth/login', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ username, password })
            });

            const data: AuthResponse = await response.json();

            if (response.ok && data.success && data.token) {
                this.setToken(data.token, remember);
            }

            return data;
        } catch (error) {
            return {
                success: false,
                message: 'Erro de conexão. Tente novamente.'
            };
        }
    }

    /**
     * Logout - remove token and redirect
     */
    static logout(): void {
        this.removeToken();
        window.location.href = '/login';
    }

    /**
     * Verify current token with server
     */
    static async verify(): Promise<boolean> {
        const token = this.getToken();
        if (!token) return false;

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
        } catch {
            return false;
        }
    }

    /**
     * Refresh token
     */
    static async refresh(): Promise<boolean> {
        const token = this.getToken();
        if (!token) return false;

        try {
            const response = await fetch('/api/auth/refresh', {
                method: 'POST',
                headers: {
                    'Authorization': `Bearer ${token}`
                }
            });

            if (response.ok) {
                const data: AuthResponse = await response.json();
                if (data.success && data.token) {
                    const storage = localStorage.getItem(this.TOKEN_STORAGE_KEY);
                    this.setToken(data.token, storage === 'local');
                    return true;
                }
            }

            return false;
        } catch {
            return false;
        }
    }

    /**
     * Fetch with authentication header
     */
    static async authFetch(url: string, options: RequestInit = {}): Promise<Response> {
        const token = this.getToken();

        const headers = new Headers(options.headers);
        if (token) {
            headers.set('Authorization', `Bearer ${token}`);
        }

        const response = await fetch(url, {
            ...options,
            headers
        });

        // If unauthorized, redirect to login
        if (response.status === 401) {
            this.removeToken();
            window.location.href = '/login';
            throw new Error('Session expired');
        }

        return response;
    }

    /**
     * Require authentication - redirect to login if not authenticated
     */
    static requireAuth(): void {
        if (!this.isAuthenticated()) {
            window.location.href = '/login';
        }
    }
}

// Export to global scope
(window as unknown as Record<string, unknown>).Auth = Auth;
