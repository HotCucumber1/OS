const searchInput = document.querySelector('.search-input');
const searchButton = document.querySelector('button');

function performSearch() {
    const query = searchInput.value.trim();

    if (query) {
        window.location.href = `results.html?words=${encodeURIComponent(query)}`;
    }
}

searchInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
        performSearch();
    }
});

searchButton.addEventListener('click', performSearch);