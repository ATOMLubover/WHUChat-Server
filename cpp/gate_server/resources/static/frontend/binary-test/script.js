document.getElementById('loadImageBtn').addEventListener('click', function () {
    // 后端API的URL
    const apiUrl = 'assets/portrait.png'; // 替换为你的后端API地址

    // 使用fetch请求数据
    fetch(apiUrl, {
        method: 'GET',
        headers: {
            'Accept': 'application/octet-stream', // 明确告诉后端需要二进制流
        },
    })
        .then(response => {
            if (!response.ok) {
                throw new Error('网络响应不正常');
            }
            return response.blob(); // 将响应转换为Blob对象
        })
        .then(blob => {
            // 创建一个URL指向Blob对象
            const imageUrl = URL.createObjectURL(blob);

            // 创建一个Image元素
            const img = new Image();
            img.src = imageUrl;

            // 当图片加载完成后显示
            img.onload = function () {
                const imageContainer = document.getElementById('imageContainer');
                imageContainer.innerHTML = ''; // 清空容器
                imageContainer.appendChild(img); // 添加图片到容器
            };

            // 如果图片加载失败
            img.onerror = function () {
                alert('图片加载失败，请检查图片格式是否正确。');
            };
        })
        .catch(error => {
            console.error('加载图片失败:', error);
            alert('图片加载失败，请稍后再试。');
        });
});