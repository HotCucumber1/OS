const urlParams = new URLSearchParams(window.location.search);
const query = urlParams.get('words');
const currentPage = parseInt(urlParams.get('page')) || 1;

const searchInput = document.getElementById('headerSearchInput');
const resultsContainer = document.getElementById('resultsList');
const paginationContainer = document.getElementById('pagination');

const itemsPerPage = 10;
const from = (currentPage - 1) * itemsPerPage;
const to = (currentPage * itemsPerPage) - 1;

if (query) {
    searchInput.value = query;
    fetchResults(query, from, to)
        .then((data) => {
            if (data.ok) {
                renderPagination(query, currentPage);
            }
        })
} else {
    resultsContainer.innerHTML = "<p>Необходимо указать параметр `words`</p>";
}

async function fetchResults(q, from, to) {
    try {
        const encodedQuery = encodeURIComponent(q);
        const url = `http://127.0.0.1:10000/list?words=${encodedQuery}&from=${from}&to=${to}`;

        const response = await fetch(url, {
            method: 'GET',
            headers: {'Content-Type': 'application/json'}
        });

        if (!response.ok) {
            throw new Error(`Ошибка: ${response.status}`);
        }

        const data = await response.json();
        renderResults(data.data);

        return response;
    } catch (error) {
        resultsContainer.innerHTML = "<p>Ошибка при загрузке данных или бэкенд недоступен.</p>";
        console.error(error);
    }
}

function renderResults(results) {
    resultsContainer.innerHTML = '';
    if (!results || results.length === 0) {
        resultsContainer.innerHTML = '<p>Ничего не найдено.</p>';
        return;
    }

    results.forEach((item, index) => {
        const div = document.createElement('div');
        div.className = 'result-item';
        div.style.animationDelay = `${index * 0.05}s`;

        div.innerHTML = `
            <a href="${item.fileUrl}">
                <span class="site-url">${item.fileUrl}</span>
                <h3>${item.fileUrl.split('/').pop() || item.fileUrl}</h3>
            </a>
            <p>File relevance: <b>${item.fileRelevant.toFixed(4)}</b></p>
        `;
        resultsContainer.appendChild(div);
    });
}

function renderPagination(q, current) {
    paginationContainer.innerHTML = '';

    for (let i = 1; i <= 10; i++) {
        const pageLink = document.createElement('a');
        pageLink.innerText = i;
        pageLink.href = `results.html?words=${encodeURIComponent(q)}&page=${i}`;

        if (i === current) {
            pageLink.classList.add('active');
        }

        paginationContainer.appendChild(pageLink);
    }
}

searchInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        const newQuery = searchInput.value.trim();
        if (newQuery) {
            window.location.href = `results.html?words=${encodeURIComponent(newQuery)}&page=1`;
        }
    }
});