const searchInput = document.querySelector('.search-input');
const searchButton = document.querySelector('button');

function performSearch() {
    const query = searchInput.value.trim();
    const formattedQuery = query.split(/\s+/).join('+');

    if (query) {
        window.location.href = `results.html?words=${encodeURIComponent(formattedQuery)}`;
    }
}

searchInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        performSearch();
    }
});

searchButton.addEventListener('click', performSearch);