const urlParams = new URLSearchParams(window.location.search);
const query = urlParams.get('words');
const searchInput = document.getElementById('headerSearchInput');
const resultsContainer = document.getElementById('resultsList');

if (query) {
    searchInput.value = query;
    fetchResults(query);
}

async function fetchResults(q) {
    try {
        const response = await fetch(`http://127.0.0.1:10000/list?words=${q}`, {
            method: 'GET',
            headers: {
                'Content-Type': 'application/json'
            }
        });

        if (!response.ok) {
            throw new Error(`Ошибка: ${response.status}`);
        }

        const data = await response.json();
        renderResults(data.data);
    } catch (error) {
        resultsContainer.innerHTML = "<p>Ошибка при загрузке данных.</p>";
    }
}

function renderResults(results) {
    resultsContainer.innerHTML = '';

    if (results.length === 0) {
        resultsContainer.innerHTML = '<p>Ничего не найдено.</p>';
        return;
    }

    results.forEach((item, index) => {
        const div = document.createElement('div');
        div.className = 'result-item';
        div.style.animationDelay = `${index * 0.1}s`;

        div.innerHTML = `
                <a href="${item.fileUrl}">
                    <span class="site-url">${item.fileUrl}</span>
                    <h3>${item.fileUrl}</h3>
                </a>
                <p>File relevance: ${item.fileRelevant}</p>
            `;
        resultsContainer.appendChild(div);
    });
}

searchInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        window.location.href = `results.html?q=${encodeURIComponent(searchInput.value)}`;
    }
});