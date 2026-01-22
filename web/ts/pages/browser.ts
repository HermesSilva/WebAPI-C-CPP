/**
 * Database Browser page functionality
 */

/* global Auth, escapeHtml */

interface TableInfo {
    schema: string;
    name: string;
}

interface ColumnInfo {
    name: string;
    type: string;
    nullable: boolean;
    isPrimaryKey: boolean;
}

interface DataResult {
    success: boolean;
    columns?: string[];
    rows?: Record<string, unknown>[];
    totalRows?: number;
    totalPages?: number;
    error?: string;
}

(function () {
    'use strict';

    // Require authentication
    Auth.requireAuth();

    // State
    let isConnected = false;
    let selectedTable: string | null = null;
    let selectedSchema: string | null = null;
    let columns: ColumnInfo[] = [];
    let currentPage = 1;
    let totalPages = 1;
    const pageSize = 50;

    // DOM Elements
    const connectBtn = document.getElementById('connectBtn') as HTMLButtonElement;
    const statusDot = document.getElementById('statusDot') as HTMLElement;
    const statusText = document.getElementById('statusText') as HTMLElement;
    const tableList = document.getElementById('tableList') as HTMLElement;
    const filterPanel = document.getElementById('filterPanel') as HTMLElement;
    const filterColumn = document.getElementById('filterColumn') as HTMLSelectElement;
    const filterValue = document.getElementById('filterValue') as HTMLInputElement;
    const btnFilter = document.getElementById('btnFilter') as HTMLButtonElement;
    const btnClearFilter = document.getElementById('btnClearFilter') as HTMLButtonElement;
    const dataPanel = document.getElementById('dataPanel') as HTMLElement;
    const pagination = document.getElementById('pagination') as HTMLElement;
    const paginationInfo = document.getElementById('paginationInfo') as HTMLElement;
    const pageInfo = document.getElementById('pageInfo') as HTMLElement;
    const btnFirst = document.getElementById('btnFirst') as HTMLButtonElement;
    const btnPrev = document.getElementById('btnPrev') as HTMLButtonElement;
    const btnNext = document.getElementById('btnNext') as HTMLButtonElement;
    const btnLast = document.getElementById('btnLast') as HTMLButtonElement;
    const btnLogout = document.getElementById('btnLogout') as HTMLElement;

    /**
     * Set connection status in UI
     */
    function setConnectionStatus(connected: boolean): void {
        isConnected = connected;

        statusDot.className = 'connection-status__dot' + (connected ? ' connection-status__dot--connected' : '');
        statusText.textContent = connected ? 'Conectado ao TFX' : 'Desconectado';
        connectBtn.textContent = connected ? 'Desconectar' : 'Conectar';
        connectBtn.className = 'btn btn--block ' + (connected ? 'btn--danger' : 'btn--primary');
    }

    /**
     * Toggle database connection
     */
    async function toggleConnection(): Promise<void> {
        if (isConnected) {
            await disconnect();
        } else {
            await connect();
        }
    }

    /**
     * Connect to database
     */
    async function connect(): Promise<void> {
        connectBtn.disabled = true;
        connectBtn.textContent = 'Conectando...';

        try {
            const response = await Auth.authFetch('/api/browseroso/connect', {
                method: 'POST'
            });

            const data = await response.json();

            if (data.success) {
                setConnectionStatus(true);
                await loadTables();
            } else {
                alert('Falha na conexão: ' + (data.message || 'Erro desconhecido'));
            }
        } catch (error) {
            alert('Erro de conexão: ' + (error instanceof Error ? error.message : 'Erro desconhecido'));
        } finally {
            connectBtn.disabled = false;
            connectBtn.textContent = isConnected ? 'Desconectar' : 'Conectar';
        }
    }

    /**
     * Disconnect from database
     */
    async function disconnect(): Promise<void> {
        try {
            await Auth.authFetch('/api/browseroso/disconnect', {
                method: 'POST'
            });
        } catch {
            // Ignore errors
        }

        setConnectionStatus(false);
        selectedTable = null;
        selectedSchema = null;

        tableList.innerHTML = `
            <div class="empty-state">
                <p>Conecte para ver as tabelas</p>
            </div>
        `;

        filterPanel.classList.add('hidden');
        pagination.classList.add('hidden');

        dataPanel.innerHTML = `
            <div class="empty-state">
                <div class="empty-state__icon">📋</div>
                <h3 class="empty-state__title">Nenhuma Tabela Selecionada</h3>
                <p>Selecione uma tabela na barra lateral para visualizar os dados</p>
            </div>
        `;
    }

    /**
     * Load tables list
     */
    async function loadTables(): Promise<void> {
        try {
            const response = await Auth.authFetch('/api/browseroso/tables');
            const data = await response.json();

            if (data.tables && data.tables.length > 0) {
                const tables: TableInfo[] = data.tables;
                tableList.innerHTML = tables.map(t => `
                    <div class="table-item" data-schema="${escapeHtml(t.schema)}" data-table="${escapeHtml(t.name)}">
                        <span class="table-item__icon">◳</span>
                        <span class="table-item__name">${escapeHtml(t.name)}</span>
                        <span class="table-item__schema">${escapeHtml(t.schema)}</span>
                    </div>
                `).join('');

                // Add click handlers
                tableList.querySelectorAll('.table-item').forEach(item => {
                    item.addEventListener('click', () => {
                        const schema = (item as HTMLElement).dataset.schema || '';
                        const table = (item as HTMLElement).dataset.table || '';
                        selectTable(schema, table, item as HTMLElement);
                    });
                });
            } else {
                tableList.innerHTML = `
                    <div class="empty-state">
                        <p>Nenhuma tabela encontrada</p>
                    </div>
                `;
            }
        } catch (error) {
            tableList.innerHTML = `
                <div class="message message--error">
                    Erro ao carregar tabelas
                </div>
            `;
        }
    }

    /**
     * Select a table
     */
    async function selectTable(schema: string, table: string, element: HTMLElement): Promise<void> {
        // Update selection
        tableList.querySelectorAll('.table-item').forEach(item => {
            item.classList.remove('table-item--active');
        });
        element.classList.add('table-item--active');

        selectedSchema = schema;
        selectedTable = table;
        currentPage = 1;

        // Show filter panel
        filterPanel.classList.remove('hidden');

        // Load columns
        try {
            const response = await Auth.authFetch(`/api/browseroso/columns?schema=${encodeURIComponent(schema)}&table=${encodeURIComponent(table)}`);
            const data = await response.json();

            columns = data.columns || [];

            // Update filter column dropdown
            filterColumn.innerHTML = '<option value="">Todas as colunas</option>' +
                columns.map(c => `<option value="${escapeHtml(c.name)}">${escapeHtml(c.name)} (${escapeHtml(c.type)})</option>`).join('');

            // Load data
            await loadData();
        } catch (error) {
            dataPanel.innerHTML = `
                <div class="message message--error">
                    Erro ao carregar colunas: ${error instanceof Error ? error.message : 'Erro desconhecido'}
                </div>
            `;
        }
    }

    /**
     * Load table data
     */
    async function loadData(): Promise<void> {
        if (!selectedTable || !selectedSchema) return;

        dataPanel.innerHTML = `
            <div class="loading">
                <div class="spinner"></div>
                <p>Carregando...</p>
            </div>
        `;

        const fc = filterColumn.value;
        const fv = filterValue.value;

        let url = `/api/browseroso/data?schema=${encodeURIComponent(selectedSchema)}&table=${encodeURIComponent(selectedTable)}&page=${currentPage}&pageSize=${pageSize}`;

        if (fc && fv) {
            url += `&filterColumn=${encodeURIComponent(fc)}&filterValue=${encodeURIComponent(fv)}`;
        }

        try {
            const response = await Auth.authFetch(url);
            const data: DataResult = await response.json();

            if (!data.success) {
                dataPanel.innerHTML = `<div class="message message--error">${escapeHtml(data.error)}</div>`;
                return;
            }

            totalPages = data.totalPages || 1;

            if (data.rows && data.rows.length > 0 && data.columns) {
                renderTable(data.columns, data.rows, data.totalRows || 0);
            } else {
                dataPanel.innerHTML = `
                    <div class="empty-state">
                        <div class="empty-state__icon">📋</div>
                        <h3 class="empty-state__title">Sem Dados</h3>
                        <p>Esta tabela está vazia ou nenhum resultado corresponde ao filtro</p>
                    </div>
                `;
                pagination.classList.add('hidden');
            }
        } catch (error) {
            dataPanel.innerHTML = `
                <div class="message message--error">
                    Erro ao carregar dados: ${error instanceof Error ? error.message : 'Erro desconhecido'}
                </div>
            `;
        }
    }

    /**
     * Render data table
     */
    function renderTable(columnNames: string[], rows: Record<string, unknown>[], totalRows: number): void {
        let html = '<table class="table"><thead><tr>';

        // Headers
        for (const col of columnNames) {
            const isPK = columns.some(c => c.name === col && c.isPrimaryKey);
            html += `<th class="${isPK ? 'table__pk' : ''}">${isPK ? '🔑 ' : ''}${escapeHtml(col)}</th>`;
        }

        html += '</tr></thead><tbody>';

        // Rows
        for (const row of rows) {
            html += '<tr>';
            for (const col of columnNames) {
                const value = row[col];
                if (value === null || value === undefined || value === 'NULL') {
                    html += '<td class="table__null">NULL</td>';
                } else {
                    html += `<td title="${escapeHtml(String(value))}">${escapeHtml(String(value))}</td>`;
                }
            }
            html += '</tr>';
        }

        html += '</tbody></table>';
        dataPanel.innerHTML = html;

        // Update pagination
        pagination.classList.remove('hidden');
        paginationInfo.textContent = `Mostrando ${(currentPage - 1) * pageSize + 1} - ${Math.min(currentPage * pageSize, totalRows)} de ${totalRows} registros`;
        pageInfo.textContent = `Página ${currentPage} de ${totalPages}`;

        btnFirst.disabled = currentPage === 1;
        btnPrev.disabled = currentPage === 1;
        btnNext.disabled = currentPage >= totalPages;
        btnLast.disabled = currentPage >= totalPages;
    }

    /**
     * Go to page
     */
    function goToPage(page: number): void {
        if (page < 1) page = 1;
        if (page > totalPages) page = totalPages;
        currentPage = page;
        loadData();
    }

    /**
     * Apply filter
     */
    function applyFilter(): void {
        currentPage = 1;
        loadData();
    }

    /**
     * Clear filter
     */
    function clearFilter(): void {
        filterColumn.value = '';
        filterValue.value = '';
        currentPage = 1;
        loadData();
    }

    /**
     * Check initial connection status
     */
    async function checkStatus(): Promise<void> {
        try {
            const response = await Auth.authFetch('/api/browseroso/status');
            const data = await response.json();

            if (data.connected) {
                setConnectionStatus(true);
                await loadTables();
            }
        } catch {
            // Not connected
        }
    }

    /**
     * Initialize
     */
    function init(): void {
        // Event listeners
        connectBtn.addEventListener('click', toggleConnection);
        btnFilter.addEventListener('click', applyFilter);
        btnClearFilter.addEventListener('click', clearFilter);
        btnLogout.addEventListener('click', (e) => {
            e.preventDefault();
            Auth.logout();
        });

        // Pagination
        btnFirst.addEventListener('click', () => goToPage(1));
        btnPrev.addEventListener('click', () => goToPage(currentPage - 1));
        btnNext.addEventListener('click', () => goToPage(currentPage + 1));
        btnLast.addEventListener('click', () => goToPage(totalPages));

        // Filter on Enter
        filterValue.addEventListener('keypress', (e) => {
            if (e.key === 'Enter') applyFilter();
        });

        // Check initial status
        checkStatus();
    }

    // Run when DOM is ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
