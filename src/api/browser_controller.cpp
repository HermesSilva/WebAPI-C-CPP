/**
 * @file browser_controller.cpp
 * @brief Database browser controller implementation
 */

#include "browser_controller.h"
#include "auth_controller.h"
#include "data/database.h"

#include <sstream>

namespace Tootega
{
namespace Api
{

void BrowserController::registerRoutes(httplib::Server &server)
{
    // Note: /browseroso page is served by StaticController
    // Here we only register the API endpoints
    server.Post("/api/browseroso/connect", connectDatabase);
    server.Post("/api/browseroso/disconnect", disconnectDatabase);
    server.Get("/api/browseroso/status", getConnectionStatus);
    server.Get("/api/browseroso/tables", getTables);
    server.Get("/api/browseroso/columns", getTableColumns);
    server.Get("/api/browseroso/data", getTableData);
}

void BrowserController::getBrowserUI(const httplib::Request &req, httplib::Response &res)
{
    // Check authentication - redirect to login if not authenticated
    std::string token = AuthController::extractToken(req);
    if (token.empty())
    {
        // Check for token in cookies or session storage (via query param for redirect)
        res.set_content(generateBrowserHTML(), "text/html; charset=utf-8");
        return;
    }
    res.set_content(generateBrowserHTML(), "text/html; charset=utf-8");
}

void BrowserController::connectDatabase(const httplib::Request &req, httplib::Response &res)
{
    // Verify JWT authentication
    if (!AuthController::verifyAuth(req, res))
    {
        return;
    }

    auto &db = Data::Database::getInstance();

    std::string connStr = "Data Source=localhost;Initial Catalog=TFX;Integrated Security=True;"
                          "Persist Security Info=False;Pooling=False;MultipleActiveResultSets=False;"
                          "Encrypt=False;TrustServerCertificate=True";

    if (!req.body.empty())
    {
        auto pos = req.body.find("\"connectionString\"");
        if (pos != std::string::npos)
        {
            auto start = req.body.find(':', pos) + 1;
            auto quote1 = req.body.find('"', start);
            auto quote2 = req.body.find('"', quote1 + 1);
            if (quote1 != std::string::npos && quote2 != std::string::npos)
            {
                connStr = req.body.substr(quote1 + 1, quote2 - quote1 - 1);
            }
        }
    }

    bool success = db.connect(connStr);

    std::ostringstream json;
    json << "{\"success\": " << (success ? "true" : "false") << ",";
    json << "\"message\": \"" << (success ? "Connected successfully" : "Connection failed") << "\",";
    json << "\"info\": \"" << db.getConnectionInfo() << "\"}";

    res.set_content(json.str(), "application/json");
}

void BrowserController::disconnectDatabase(const httplib::Request &req, httplib::Response &res)
{
    if (!AuthController::verifyAuth(req, res))
        return;

    Data::Database::getInstance().disconnect();
    res.set_content("{\"success\": true, \"message\": \"Disconnected\"}", "application/json");
}

void BrowserController::getConnectionStatus(const httplib::Request &req, httplib::Response &res)
{
    if (!AuthController::verifyAuth(req, res))
        return;

    auto &db = Data::Database::getInstance();
    std::ostringstream json;
    json << "{\"connected\": " << (db.isConnected() ? "true" : "false") << ",";
    json << "\"info\": \"" << db.getConnectionInfo() << "\"}";
    res.set_content(json.str(), "application/json");
}

void BrowserController::getTables(const httplib::Request &req, httplib::Response &res)
{
    if (!AuthController::verifyAuth(req, res))
        return;

    auto &db = Data::Database::getInstance();

    if (!db.isConnected())
    {
        res.set_content("{\"error\": \"Not connected\"}", "application/json");
        res.status = 400;
        return;
    }

    auto tables = db.getTables();
    std::ostringstream json;
    json << "{\"tables\": [";
    for (size_t i = 0; i < tables.size(); i++)
    {
        if (i > 0)
            json << ",";
        json << "{\"schema\": \"" << tables[i].schema << "\",\"name\": \"" << tables[i].name << "\"}";
    }
    json << "]}";
    res.set_content(json.str(), "application/json");
}

void BrowserController::getTableColumns(const httplib::Request &req, httplib::Response &res)
{
    if (!AuthController::verifyAuth(req, res))
        return;

    auto &db = Data::Database::getInstance();

    if (!db.isConnected())
    {
        res.set_content("{\"error\": \"Not connected\"}", "application/json");
        res.status = 400;
        return;
    }

    std::string schema = req.get_param_value("schema");
    std::string table = req.get_param_value("table");

    if (table.empty())
    {
        res.set_content("{\"error\": \"Table name required\"}", "application/json");
        res.status = 400;
        return;
    }

    auto columns = db.getColumns(schema, table);
    std::ostringstream json;
    json << "{\"columns\": [";
    for (size_t i = 0; i < columns.size(); i++)
    {
        if (i > 0)
            json << ",";
        json << "{\"name\": \"" << columns[i].name << "\",";
        json << "\"type\": \"" << columns[i].type << "\",";
        json << "\"nullable\": " << (columns[i].nullable ? "true" : "false") << ",";
        json << "\"isPrimaryKey\": " << (columns[i].isPrimaryKey ? "true" : "false") << "}";
    }
    json << "]}";
    res.set_content(json.str(), "application/json");
}

void BrowserController::getTableData(const httplib::Request &req, httplib::Response &res)
{
    if (!AuthController::verifyAuth(req, res))
        return;

    auto &db = Data::Database::getInstance();

    if (!db.isConnected())
    {
        res.set_content("{\"error\": \"Not connected\"}", "application/json");
        res.status = 400;
        return;
    }

    std::string schema = req.get_param_value("schema");
    std::string table = req.get_param_value("table");
    std::string filterColumn = req.get_param_value("filterColumn");
    std::string filterValue = req.get_param_value("filterValue");

    int page = 1, pageSize = 50;
    if (!req.get_param_value("page").empty())
        page = std::stoi(req.get_param_value("page"));
    if (!req.get_param_value("pageSize").empty())
        pageSize = std::stoi(req.get_param_value("pageSize"));

    if (table.empty())
    {
        res.set_content("{\"error\": \"Table name required\"}", "application/json");
        res.status = 400;
        return;
    }

    auto result = db.selectData(schema, table, filterColumn, filterValue, page, pageSize);

    std::ostringstream json;
    json << "{\"success\": " << (result.success ? "true" : "false") << ",";

    if (!result.success)
    {
        std::string escapedError;
        for (char c : result.error)
        {
            if (c == '"')
                escapedError += "\\\"";
            else if (c == '\\')
                escapedError += "\\\\";
            else if (c == '\n')
                escapedError += "\\n";
            else if (c == '\r')
                escapedError += "\\r";
            else
                escapedError += c;
        }
        json << "\"error\": \"" << escapedError << "\"}";
    }
    else
    {
        json << "\"totalRows\": " << result.totalRows << ",";
        json << "\"page\": " << page << ",\"pageSize\": " << pageSize << ",";
        json << "\"totalPages\": " << ((result.totalRows + pageSize - 1) / pageSize) << ",";
        json << "\"columns\": [";
        for (size_t i = 0; i < result.columns.size(); i++)
        {
            if (i > 0)
                json << ",";
            json << "\"" << result.columns[i] << "\"";
        }
        json << "],\"rows\": [";
        for (size_t r = 0; r < result.rows.size(); r++)
        {
            if (r > 0)
                json << ",";
            json << "{";
            for (size_t c = 0; c < result.rows[r].size(); c++)
            {
                if (c > 0)
                    json << ",";
                std::string escapedValue;
                for (char ch : result.rows[r][c].second)
                {
                    if (ch == '"')
                        escapedValue += "\\\"";
                    else if (ch == '\\')
                        escapedValue += "\\\\";
                    else if (ch == '\n')
                        escapedValue += "\\n";
                    else if (ch == '\r')
                        escapedValue += "\\r";
                    else if (ch == '\t')
                        escapedValue += "\\t";
                    else
                        escapedValue += ch;
                }
                json << "\"" << result.rows[r][c].first << "\": \"" << escapedValue << "\"";
            }
            json << "}";
        }
        json << "]}";
    }

    res.set_content(json.str(), "application/json");
}

std::string BrowserController::generateBrowserHTML()
{
    std::string html;
    html += "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\"><meta name=\"viewport\" "
            "content=\"width=device-width,initial-scale=1.0\"><title>Tootega - Database Browser</title><style>";
    html += "*{margin:0;padding:0;box-sizing:border-box}body{font-family:'Segoe "
            "UI',system-ui,sans-serif;background:#0f0f23;color:#e0e0e0;min-height:100vh}";
    html += ".header{background:linear-gradient(135deg,#1a2a4a 0%,#1e3a5f 100%);padding:1rem "
            "2rem;display:flex;justify-content:space-between;align-items:center;border-bottom:1px solid #333}";
    html += ".header h1{font-size:1.5rem;color:#fff}.header h1 span{color:#4da6ff}.nav-links{display:flex;gap:1rem}";
    html += ".nav-links a{color:#aaa;text-decoration:none;padding:0.5rem 1rem;border-radius:6px;transition:all "
            "0.2s}.nav-links a:hover{background:rgba(255,255,255,0.1);color:#fff}";
    html +=
        ".container{display:flex;height:calc(100vh - 60px)}.sidebar{width:300px;background:#1a1a2e;border-right:1px "
        "solid #333;display:flex;flex-direction:column}";
    html += ".connection-panel{padding:1rem;border-bottom:1px solid #333}.connection-panel "
            "h3{font-size:0.9rem;color:#888;margin-bottom:0.75rem;text-transform:uppercase}";
    html += ".status-indicator{display:flex;align-items:center;gap:0.5rem;margin-bottom:1rem;padding:0.5rem;background:"
            "rgba(0,0,0,0.2);border-radius:6px}";
    html += ".status-dot{width:10px;height:10px;border-radius:50%;background:#ef4444}.status-dot.connected{background:#"
            "22c55e}";
    html += ".btn{padding:0.6rem 1rem;border:none;border-radius:6px;cursor:pointer;font-size:0.875rem;transition:all "
            "0.2s;width:100%}";
    html += ".btn-primary{background:#2d7dd2;color:white}.btn-primary:hover{background:#1e5aa8}.btn-danger{background:#"
            "ef4444;color:white}.btn-danger:hover{background:#dc2626}";
    html +=
        ".tables-panel{flex:1;overflow-y:auto;padding:1rem}.tables-panel "
        "h3{font-size:0.9rem;color:#888;margin-bottom:0.75rem;text-transform:uppercase}.table-list{list-style:none}";
    html += ".table-item{padding:0.6rem "
            "0.75rem;cursor:pointer;border-radius:6px;display:flex;align-items:center;gap:0.5rem;transition:all "
            "0.2s;font-size:0.875rem}";
    html +=
        ".table-item:hover{background:rgba(45,125,210,0.2)}.table-item.active{background:rgba(45,125,210,0.3);color:#"
        "6bb8ff}.table-item .schema{color:#666;font-size:0.75rem;margin-left:auto}.table-icon{color:#2d7dd2}";
    html += ".main-content{flex:1;display:flex;flex-direction:column;overflow:hidden}";
    html += ".filter-panel{padding:1rem 1.5rem;background:#16162d;border-bottom:1px solid "
            "#333;display:flex;gap:1rem;align-items:flex-end}";
    html += ".filter-group{display:flex;flex-direction:column;gap:0.25rem}.filter-group "
            "label{font-size:0.75rem;color:#888;text-transform:uppercase}";
    html += ".filter-group select,.filter-group input{padding:0.6rem 0.75rem;background:#1a1a2e;border:1px solid "
            "#333;border-radius:6px;color:#e0e0e0;font-size:0.875rem;min-width:200px}";
    html += ".filter-group select:focus,.filter-group input:focus{outline:none;border-color:#2d7dd2}";
    html += ".btn-filter{background:#2d7dd2;color:white;padding:0.6rem "
            "1.5rem}.btn-filter:hover{background:#1e5aa8}.btn-clear{background:#333;color:#aaa;padding:0.6rem "
            "1rem}.btn-clear:hover{background:#444;color:#fff}";
    html += ".data-panel{flex:1;overflow:auto;padding:1rem "
            "1.5rem}.data-table{width:100%;border-collapse:collapse;font-size:0.875rem}";
    html += ".data-table th{background:#1a1a2e;padding:0.75rem 1rem;text-align:left;border-bottom:2px solid "
            "#2d7dd2;position:sticky;top:0;font-weight:600;color:#6bb8ff}";
    html += ".data-table td{padding:0.6rem 1rem;border-bottom:1px solid "
            "#2a2a4a;max-width:300px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}";
    html += ".data-table tr:hover "
            "td{background:rgba(45,125,210,0.1)}.null-value{color:#666;font-style:italic}.pk-column{color:#fbbf24}";
    html += ".pagination{padding:1rem 1.5rem;background:#16162d;border-top:1px solid "
            "#333;display:flex;justify-content:space-between;align-items:center}";
    html += ".pagination-info{color:#888;font-size:0.875rem}.pagination-buttons{display:flex;gap:0.5rem}";
    html += ".pagination-buttons button{padding:0.5rem 1rem;background:#1a1a2e;border:1px solid "
            "#333;border-radius:6px;color:#e0e0e0;cursor:pointer;transition:all 0.2s}";
    html +=
        ".pagination-buttons button:hover:not(:disabled){background:#2a2a4a;border-color:#2d7dd2}.pagination-buttons "
        "button:disabled{opacity:0.5;cursor:not-allowed}";
    html += ".empty-state{text-align:center;padding:4rem 2rem;color:#666}.empty-state "
            "h3{margin-bottom:0.5rem;color:#888}.loading{text-align:center;padding:2rem;color:#9d4edd}";
    html += ".error-message{background:rgba(239,68,68,0.2);border:1px solid "
            "#ef4444;padding:1rem;border-radius:6px;margin:1rem;color:#fca5a5}";
    html += "@keyframes spin{to{transform:rotate(360deg)}}.spinner{width:24px;height:24px;border:3px solid "
            "#333;border-top-color:#2d7dd2;border-radius:50%;animation:spin 1s linear infinite;display:inline-block}";
    html += "</style></head><body>";
    html += "<header class=\"header\"><h1>Tootega <span>Database Browser</span></h1><nav class=\"nav-links\"><a "
            "href=\"/\">Home</a><a href=\"/api/docs\">API Docs</a><a href=\"/browseroso\" "
            "class=\"active\">Browser</a></nav></header>";
    html += "<div class=\"container\"><aside class=\"sidebar\"><div class=\"connection-panel\"><h3>Connection</h3><div "
            "class=\"status-indicator\"><div class=\"status-dot\" id=\"statusDot\"></div><span "
            "id=\"statusText\">Disconnected</span></div>";
    html += "<button class=\"btn btn-primary\" id=\"connectBtn\" onclick=\"toggleConnection()\">Connect</button></div>";
    html += "<div class=\"tables-panel\"><h3>Tables</h3><ul class=\"table-list\" id=\"tableList\"><li "
            "class=\"empty-state\" style=\"padding:1rem;font-size:0.875rem;\">Connect to view "
            "tables</li></ul></div></aside>";
    html += "<main class=\"main-content\"><div class=\"filter-panel\" id=\"filterPanel\" style=\"display:none;\"><div "
            "class=\"filter-group\"><label>Filter Column</label><select id=\"filterColumn\"><option value=\"\">All "
            "columns</option></select></div>";
    html += "<div class=\"filter-group\"><label>Filter Value</label><input type=\"text\" id=\"filterValue\" "
            "placeholder=\"Search...\" onkeypress=\"if(event.key==='Enter')applyFilter()\"></div>";
    html += "<button class=\"btn btn-filter\" onclick=\"applyFilter()\">Filter</button><button class=\"btn btn-clear\" "
            "onclick=\"clearFilter()\">Clear</button></div>";
    html += "<div class=\"data-panel\" id=\"dataPanel\"><div class=\"empty-state\"><h3>No Table Selected</h3><p>Select "
            "a table from the sidebar to view its data</p></div></div>";
    html += "<div class=\"pagination\" id=\"pagination\" style=\"display:none;\"><div class=\"pagination-info\" "
            "id=\"paginationInfo\">Showing 0 of 0 rows</div><div class=\"pagination-buttons\">";
    html += "<button onclick=\"goToPage(1)\" id=\"btnFirst\">First</button><button onclick=\"goToPage(currentPage-1)\" "
            "id=\"btnPrev\">Previous</button>";
    html += "<span id=\"pageInfo\" style=\"padding:0.5rem 1rem;color:#888;\">Page 1</span>";
    html += "<button onclick=\"goToPage(currentPage+1)\" id=\"btnNext\">Next</button><button "
            "onclick=\"goToPage(totalPages)\" id=\"btnLast\">Last</button></div></div></main></div>";
    html += "<script>";
    // Auth check and helper function
    html += "var authToken = localStorage.getItem('jwt_token') || sessionStorage.getItem('jwt_token');";
    html += "if(!authToken){window.location.href='/login';throw new Error('Not authenticated');}";
    html +=
        "function "
        "authFetch(url,options){options=options||{};options.headers=options.headers||{};options.headers['Authorization'"
        "]='Bearer '+authToken;return "
        "fetch(url,options).then(function(r){if(r.status===401){localStorage.removeItem('jwt_token');sessionStorage."
        "removeItem('jwt_token');window.location.href='/login';throw new Error('Session expired');}return r;});}";
    html +=
        "var "
        "isConnected=false,selectedTable=null,selectedSchema=null,columns=[],currentPage=1,totalPages=1,pageSize=50;";
    html +=
        "function toggleConnection(){var "
        "b=document.getElementById('connectBtn');if(isConnected){authFetch('/api/browseroso/"
        "disconnect',{method:'POST'}).then(function(){setConnectionStatus(false);document.getElementById('tableList')."
        "innerHTML='<li class=\"empty-state\" style=\"padding:1rem;font-size:0.875rem;\">Connect to view "
        "tables</"
        "li>';document.getElementById('filterPanel').style.display='none';document.getElementById('pagination').style."
        "display='none';document.getElementById('dataPanel').innerHTML='<div class=\"empty-state\"><h3>No Table "
        "Selected</h3><p>Select a table from the sidebar to view its "
        "data</p></div>';selectedTable=null;})}else{b.disabled=true;b.textContent='Connecting...';authFetch('/api/"
        "browseroso/connect',{method:'POST'}).then(function(r){return "
        "r.json()}).then(function(d){if(d.success){setConnectionStatus(true);loadTables()}else{alert('Connection "
        "failed: "
        "'+d.message)}b.disabled=false;b.textContent=isConnected?'Disconnect':'Connect'}).catch(function(e){alert('"
        "Connection error: '+e.message);b.disabled=false;b.textContent='Connect'})}}";
    html += "function "
            "setConnectionStatus(c){isConnected=c;document.getElementById('statusDot').className='status-dot'+(c?' "
            "connected':'');document.getElementById('statusText').textContent=c?'Connected to "
            "TFX':'Disconnected';document.getElementById('connectBtn').textContent=c?'Disconnect':'Connect';document."
            "getElementById('connectBtn').className='btn '+(c?'btn-danger':'btn-primary')}";
    html += "function loadTables(){authFetch('/api/browseroso/tables').then(function(r){return "
            "r.json()}).then(function(d){var "
            "l=document.getElementById('tableList');if(d.tables&&d.tables.length>0){var h='';for(var "
            "i=0;i<d.tables.length;i++){var t=d.tables[i];h+='<li class=\"table-item\" "
            "onclick=\"selectTable(\\''+t.schema+'\\',\\''+t.name+'\\')\"><span "
            "class=\"table-icon\">&#9635;</span><span>'+t.name+'</span><span "
            "class=\"schema\">'+t.schema+'</span></li>'}l.innerHTML=h}else{l.innerHTML='<li class=\"empty-state\" "
            "style=\"padding:1rem;font-size:0.875rem;\">No tables found</li>'}})}";
    html +=
        "function selectTable(s,n){var items=document.querySelectorAll('.table-item');for(var "
        "i=0;i<items.length;i++){items[i].classList.remove('active')}event.currentTarget.classList.add('active');"
        "selectedTable=n;selectedSchema=s;currentPage=1;document.getElementById('filterPanel').style.display='flex';"
        "authFetch('/api/browseroso/columns?schema='+s+'&table='+n).then(function(r){return "
        "r.json()}).then(function(d){columns=d.columns||[];var f=document.getElementById('filterColumn');var "
        "h='<option value=\"\">All columns</option>';for(var i=0;i<columns.length;i++){h+='<option "
        "value=\"'+columns[i].name+'\">'+columns[i].name+' ('+columns[i].type+')</option>'}f.innerHTML=h;loadData()})}";
    html +=
        "function loadData(){var p=document.getElementById('dataPanel');p.innerHTML='<div class=\"loading\"><div "
        "class=\"spinner\"></div><p>Loading...</p></div>';var fc=document.getElementById('filterColumn').value;var "
        "fv=document.getElementById('filterValue').value;var "
        "u='/api/browseroso/"
        "data?schema='+selectedSchema+'&table='+selectedTable+'&page='+currentPage+'&pageSize='+pageSize;if(fc&&fv){u+="
        "'&filterColumn='+encodeURIComponent(fc)+'&filterValue='+encodeURIComponent(fv)}authFetch(u).then(function(r){"
        "return r.json()}).then(function(d){if(!d.success){p.innerHTML='<div "
        "class=\"error-message\">'+d.error+'</div>';return}totalPages=d.totalPages||1;if(d.rows&&d.rows.length>0){var "
        "h='<table class=\"data-table\"><thead><tr>';for(var i=0;i<d.columns.length;i++){var c=d.columns[i];var "
        "pk=false;for(var "
        "j=0;j<columns.length;j++){if(columns[j].name===c&&columns[j].isPrimaryKey){pk=true;break}}h+='<th "
        "class=\"'+(pk?'pk-column':'')+'\">'+(pk?'&#128273; ':'')+c+'</th>'}h+='</tr></thead><tbody>';for(var "
        "r=0;r<d.rows.length;r++){h+='<tr>';for(var c=0;c<d.columns.length;c++){var "
        "v=d.rows[r][d.columns[c]];if(v==='NULL'||v===null||v===undefined){h+='<td "
        "class=\"null-value\">NULL</td>'}else{h+='<td "
        "title=\"'+escapeHtml(v)+'\">'+escapeHtml(v)+'</td>'}}h+='</tr>'}h+='</tbody></"
        "table>';p.innerHTML=h;document.getElementById('pagination').style.display='flex';document.getElementById('"
        "paginationInfo').textContent='Showing '+((currentPage-1)*pageSize+1)+' - "
        "'+Math.min(currentPage*pageSize,d.totalRows)+' of '+d.totalRows+' "
        "rows';document.getElementById('pageInfo').textContent='Page '+currentPage+' of "
        "'+totalPages;document.getElementById('btnFirst').disabled=currentPage===1;document.getElementById('btnPrev')."
        "disabled=currentPage===1;document.getElementById('btnNext').disabled=currentPage>=totalPages;document."
        "getElementById('btnLast').disabled=currentPage>=totalPages}else{p.innerHTML='<div "
        "class=\"empty-state\"><h3>No Data</h3><p>This table is empty or no results match your "
        "filter</p></"
        "div>';document.getElementById('pagination').style.display='none'}}).catch(function(e){p.innerHTML='<div "
        "class=\"error-message\">Error loading data: '+e.message+'</div>'})}";
    html += "function applyFilter(){currentPage=1;loadData()}function "
            "clearFilter(){document.getElementById('filterColumn').value='';document.getElementById('filterValue')."
            "value='';currentPage=1;loadData()}function "
            "goToPage(p){if(p<1)p=1;if(p>totalPages)p=totalPages;currentPage=p;loadData()}";
    html += "function escapeHtml(t){if(t===null||t===undefined)return'';var "
            "d=document.createElement('div');d.textContent=String(t);return d.innerHTML}";
    html += "authFetch('/api/browseroso/status').then(function(r){return "
            "r.json()}).then(function(d){if(d.connected){setConnectionStatus(true);loadTables()}});";
    html += "</script></body></html>";
    return html;
}

} // namespace Api
} // namespace Tootega
