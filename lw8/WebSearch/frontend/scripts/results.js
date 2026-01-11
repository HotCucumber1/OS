const urlParams = new URLSearchParams(window.location.search);
const query = urlParams.get('q');
const searchInput = document.getElementById('headerSearchInput');
const resultsContainer = document.getElementById('resultsList');

if (query) {
    searchInput.value = query;
    fetchResults(query);
}

async function fetchResults(q) {
    try {
        await new Promise(resolve => setTimeout(resolve, 800));

        // Здесь должен быть ваш реальный API:
        // const response = await fetch(`https://api.example.com/search?q=${q}`);
        // const data = await response.json();

        // Заглушка данных (mock data)
        const mockData = [
            {
                title: "Что такое " + q + "?",
                url: "https://wikipedia.org/wiki/" + q,
                desc: "Подробное описание вашего запроса в энциклопедии. История, факты и текущее состояние."
            },
            {
                title: "Купить " + q + " по выгодной цене",
                url: "https://shop.example.com/search?item=" + q,
                desc: "Большой выбор товаров по запросу " + q + ". Быстрая доставка и гарантия качества."
            },
            {
                title: "Топ 10 советов по " + q,
                url: "https://blog.dev/posts/top-tips",
                desc: "Профессиональные рекомендации и лайфхаки, которые помогут вам лучше разобраться в теме."
            }
        ];

        renderResults(mockData);
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
                <a href="${item.url}">
                    <span class="site-url">${item.url}</span>
                    <h3>${item.title}</h3>
                </a>
                <p>${item.desc}</p>
            `;
        resultsContainer.appendChild(div);
    });
}

searchInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        window.location.href = `results.html?q=${encodeURIComponent(searchInput.value)}`;
    }
});