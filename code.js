window.onload = function()
{
    checkSumButton.addEventListener('click', function()
    {
        prevStyle = window.getComputedStyle(document.getElementById('checkSumContainer')).display;
        console.log("display: '"+prevStyle+"'");
        checkSumContainer.style['display'] = prevStyle === 'none'?'block':'none';
        checkSumButton.innerText = prevStyle === 'none'? 'v' : '>';
    });
};