const form = document.getElementById('addContentForm');
const btn = document.getElementById('submitBtn');
const msg = document.getElementById('message');

form.addEventListener('submit', async (e) => {
    e.preventDefault();

    const formData = {
        url: document.getElementById('url').value,
        title: document.getElementById('title').value,
        content: document.getElementById('content').value
    };

    btn.disabled = true;
    btn.textContent = 'Отправка...';
    msg.style.display = 'none';

    try {
        const response = await simulateFetch('https://api.lumina.search/v1/index', formData);

        if (response.ok) {
            showStatus('Страница успешно добавлена в индекс!', 'success');
            form.reset();
        } else {
            throw new Error('Ошибка сервера');
        }
    } catch (err) {
        showStatus('Произошла ошибка при отправке. Попробуйте позже.', 'error');
    } finally {
        btn.disabled = false;
        btn.textContent = 'Добавить в базу';
    }
});

function simulateFetch(url, data) {
    console.log('Отправка данных на:', url, data);

    return new Promise((resolve) => {
        setTimeout(() => {
            resolve({ok: true});
        }, 500);
    });
}

function showStatus(text, type) {
    msg.textContent = text;
    msg.className = '';
    msg.classList.add(type);
    msg.style.display = 'block';
}