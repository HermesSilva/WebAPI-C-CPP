(function () {
    'use strict';
    Auth.requireAuth();
    const tabId = 'tab_' + Date.now() + '_' + Math.random().toString(36).substring(2, 11);
    async function tabFetch(url, options = {}) {
        const separator = url.includes('?') ? '&' : '?';
        const urlWithTab = url + separator + 'tabId=' + encodeURIComponent(tabId);
        return Auth.authFetch(urlWithTab, options);
    }
    let isConnected = false;
    let selectedDatabase = null;
    let selectedTable = null;
    let selectedSchema = null;
    let columns = [];
    let currentPage = 1;
    let totalPages = 1;
    const pageSize = 50;
    const connectBtn = document.getElementById('connectBtn');
    const statusDot = document.getElementById('statusDot');
    const statusText = document.getElementById('statusText');
    const databaseSection = document.getElementById('databaseSection');
    const databaseSelect = document.getElementById('databaseSelect');
    const tableList = document.getElementById('tableList');
    const filterPanel = document.getElementById('filterPanel');
    const filterColumn = document.getElementById('filterColumn');
    const filterValue = document.getElementById('filterValue');
    const btnFilter = document.getElementById('btnFilter');
    const btnClearFilter = document.getElementById('btnClearFilter');
    const dataPanel = document.getElementById('dataPanel');
    const pagination = document.getElementById('pagination');
    const paginationInfo = document.getElementById('paginationInfo');
    const pageInfo = document.getElementById('pageInfo');
    const btnFirst = document.getElementById('btnFirst');
    const btnPrev = document.getElementById('btnPrev');
    const btnNext = document.getElementById('btnNext');
    const btnLast = document.getElementById('btnLast');
    const btnLogout = document.getElementById('btnLogout');
    function setConnectionStatus(connected) {
        isConnected = connected;
        statusDot.className = 'connection-status__dot' + (connected ? ' connection-status__dot--connected' : '');
        statusText.textContent = connected ? 'Conectado ao servidor' : 'Desconectado';
        connectBtn.textContent = connected ? 'Desconectar' : 'Conectar';
        connectBtn.className = 'btn btn--block ' + (connected ? 'btn--danger' : 'btn--primary');
        if (connected) {
            databaseSection.classList.remove('hidden');
        }
        else {
            databaseSection.classList.add('hidden');
            databaseSelect.innerHTML = '<option value="">Selecione uma base...</option>';
        }
    }
    async function toggleConnection() {
        if (isConnected) {
            await disconnect();
        }
        else {
            await connect();
        }
    }
    async function connect() {
        connectBtn.disabled = true;
        connectBtn.textContent = 'Conectando...';
        try {
            const response = await tabFetch('/api/browseroso/connect', {
                method: 'POST'
            });
            const data = await response.json();
            if (data.success) {
                setConnectionStatus(true);
                await loadDatabases();
            }
            else {
                alert('Falha na conexão: ' + (data.message || 'Erro desconhecido'));
            }
        }
        catch (error) {
            alert('Erro de conexão: ' + (error instanceof Error ? error.message : 'Erro desconhecido'));
        }
        finally {
            connectBtn.disabled = false;
            connectBtn.textContent = isConnected ? 'Desconectar' : 'Conectar';
        }
    }
    async function disconnect() {
        try {
            await tabFetch('/api/browseroso/disconnect', {
                method: 'POST'
            });
        }
        catch {
        }
        setConnectionStatus(false);
        selectedDatabase = null;
        selectedTable = null;
        selectedSchema = null;
        tableList.innerHTML = `
            <div class="empty-state">
                <p>Selecione uma base de dados</p>
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
    async function loadDatabases() {
        try {
            const response = await tabFetch('/api/browseroso/databases');
            const data = await response.json();
            if (data.databases && data.databases.length > 0) {
                const databases = data.databases;
                databaseSelect.innerHTML = '<option value="">Selecione uma base...</option>' +
                    databases.map(db => `<option value="${escapeHtml(db)}">${escapeHtml(db)}</option>`).join('');
            }
            else {
                databaseSelect.innerHTML = '<option value="">Nenhuma base encontrada</option>';
            }
        }
        catch (error) {
            databaseSelect.innerHTML = '<option value="">Erro ao carregar bases</option>';
        }
    }
    async function changeDatabase(databaseName) {
        if (!databaseName) {
            selectedDatabase = null;
            tableList.innerHTML = `
                <div class="empty-state">
                    <p>Selecione uma base de dados</p>
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
            return;
        }
        tableList.innerHTML = `
            <div class="loading">
                <div class="spinner"></div>
                <p>Carregando...</p>
            </div>
        `;
        try {
            const response = await tabFetch('/api/browseroso/database', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ database: databaseName })
            });
            const data = await response.json();
            if (data.success) {
                selectedDatabase = databaseName;
                statusText.textContent = `Conectado: ${databaseName}`;
                await loadTables();
            }
            else {
                alert('Falha ao mudar de base: ' + (data.message || 'Erro desconhecido'));
                tableList.innerHTML = `
                    <div class="empty-state">
                        <p>Selecione uma base de dados</p>
                    </div>
                `;
            }
        }
        catch (error) {
            alert('Erro: ' + (error instanceof Error ? error.message : 'Erro desconhecido'));
            tableList.innerHTML = `
                <div class="empty-state">
                    <p>Selecione uma base de dados</p>
                </div>
            `;
        }
    }
    async function loadTables() {
        try {
            const response = await tabFetch('/api/browseroso/tables');
            const data = await response.json();
            if (data.tables && data.tables.length > 0) {
                const tables = data.tables;
                tableList.innerHTML = tables.map(t => `
                    <div class="table-item" data-schema="${escapeHtml(t.schema)}" data-table="${escapeHtml(t.name)}">
                        <span class="table-item__icon">◳</span>
                        <span class="table-item__name">${escapeHtml(t.name)}</span>
                        <span class="table-item__schema">${escapeHtml(t.schema)}</span>
                    </div>
                `).join('');
                tableList.querySelectorAll('.table-item').forEach(item => {
                    item.addEventListener('click', () => {
                        const schema = item.dataset.schema || '';
                        const table = item.dataset.table || '';
                        selectTable(schema, table, item);
                    });
                });
            }
            else {
                tableList.innerHTML = `
                    <div class="empty-state">
                        <p>Nenhuma tabela encontrada</p>
                    </div>
                `;
            }
        }
        catch (error) {
            tableList.innerHTML = `
                <div class="message message--error">
                    Erro ao carregar tabelas
                </div>
            `;
        }
    }
    async function selectTable(schema, table, element) {
        tableList.querySelectorAll('.table-item').forEach(item => {
            item.classList.remove('table-item--active');
        });
        element.classList.add('table-item--active');
        selectedSchema = schema;
        selectedTable = table;
        currentPage = 1;
        filterPanel.classList.remove('hidden');
        try {
            const response = await tabFetch(`/api/browseroso/columns?schema=${encodeURIComponent(schema)}&table=${encodeURIComponent(table)}`);
            const data = await response.json();
            columns = data.columns || [];
            filterColumn.innerHTML = '<option value="">Todas as colunas</option>' +
                columns.map(c => `<option value="${escapeHtml(c.name)}">${escapeHtml(c.name)} (${escapeHtml(c.type)})</option>`).join('');
            await loadData();
        }
        catch (error) {
            dataPanel.innerHTML = `
                <div class="message message--error">
                    Erro ao carregar colunas: ${error instanceof Error ? error.message : 'Erro desconhecido'}
                </div>
            `;
        }
    }
    async function loadData() {
        if (!selectedTable || !selectedSchema)
            return;
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
            const response = await tabFetch(url);
            const data = await response.json();
            if (!data.success) {
                dataPanel.innerHTML = `<div class="message message--error">${escapeHtml(data.error)}</div>`;
                return;
            }
            totalPages = data.totalPages || 1;
            if (data.rows && data.rows.length > 0 && data.columns) {
                renderTable(data.columns, data.rows, data.totalRows || 0);
            }
            else {
                dataPanel.innerHTML = `
                    <div class="empty-state">
                        <div class="empty-state__icon">📋</div>
                        <h3 class="empty-state__title">Sem Dados</h3>
                        <p>Esta tabela está vazia ou nenhum resultado corresponde ao filtro</p>
                    </div>
                `;
                pagination.classList.add('hidden');
            }
        }
        catch (error) {
            dataPanel.innerHTML = `
                <div class="message message--error">
                    Erro ao carregar dados: ${error instanceof Error ? error.message : 'Erro desconhecido'}
                </div>
            `;
        }
    }
    function renderTable(columnNames, rows, totalRows) {
        let html = '<table class="table"><thead><tr>';
        for (const col of columnNames) {
            const isPK = columns.some(c => c.name === col && c.isPrimaryKey);
            html += `<th class="${isPK ? 'table__pk' : ''}">${isPK ? '🔑 ' : ''}${escapeHtml(col)}</th>`;
        }
        html += '</tr></thead><tbody>';
        for (const row of rows) {
            html += '<tr>';
            for (const col of columnNames) {
                const value = row[col];
                if (value === null || value === undefined || value === 'NULL') {
                    html += '<td class="table__null">NULL</td>';
                }
                else {
                    html += `<td title="${escapeHtml(String(value))}">${escapeHtml(String(value))}</td>`;
                }
            }
            html += '</tr>';
        }
        html += '</tbody></table>';
        dataPanel.innerHTML = html;
        pagination.classList.remove('hidden');
        paginationInfo.textContent = `Mostrando ${(currentPage - 1) * pageSize + 1} - ${Math.min(currentPage * pageSize, totalRows)} de ${totalRows} registros`;
        pageInfo.textContent = `Página ${currentPage} de ${totalPages}`;
        btnFirst.disabled = currentPage === 1;
        btnPrev.disabled = currentPage === 1;
        btnNext.disabled = currentPage >= totalPages;
        btnLast.disabled = currentPage >= totalPages;
    }
    function goToPage(page) {
        if (page < 1)
            page = 1;
        if (page > totalPages)
            page = totalPages;
        currentPage = page;
        loadData();
    }
    function applyFilter() {
        currentPage = 1;
        loadData();
    }
    function clearFilter() {
        filterColumn.value = '';
        filterValue.value = '';
        currentPage = 1;
        loadData();
    }
    async function checkStatus() {
        try {
            const response = await tabFetch('/api/browseroso/status');
            const data = await response.json();
            if (data.connected) {
                setConnectionStatus(true);
                await loadDatabases();
            }
        }
        catch {
        }
    }
    function init() {
        connectBtn.addEventListener('click', toggleConnection);
        databaseSelect.addEventListener('change', () => changeDatabase(databaseSelect.value));
        btnFilter.addEventListener('click', applyFilter);
        btnClearFilter.addEventListener('click', clearFilter);
        btnLogout.addEventListener('click', (e) => {
            e.preventDefault();
            Auth.logout();
        });
        btnFirst.addEventListener('click', () => goToPage(1));
        btnPrev.addEventListener('click', () => goToPage(currentPage - 1));
        btnNext.addEventListener('click', () => goToPage(currentPage + 1));
        btnLast.addEventListener('click', () => goToPage(totalPages));
        filterValue.addEventListener('keypress', (e) => {
            if (e.key === 'Enter')
                applyFilter();
        });
        checkStatus();
    }
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    }
    else {
        init();
    }
})();
//# sourceMappingURL=browseroso.js.map