document.addEventListener("DOMContentLoaded", function() {
  const ssidSelect = document.getElementById("ssid");
  const spinner = document.getElementById("ssid-spinner");

  const currentSsid = ssidSelect.getAttribute("current-data");

  ssidSelect.classList.add("has-spinner");

  fetch("/scan")
    .then(res => res.json())
    .then(data => {
      ssidSelect.innerHTML = "";
      data.forEach(ssid => {
        const option = document.createElement("option");
        option.value = ssid;
        option.textContent = ssid;

        if (ssid === currentSsid) {
          option.selected = true;
        }

        ssidSelect.appendChild(option);
      });
      ssidSelect.disabled = false;
      spinner.classList.add("hidden");
      ssidSelect.classList.remove("has-spinner");
    })
    .catch(err => {
      ssidSelect.innerHTML = "<option>Error scanning Wi-Fi</option>";
      spinner.classList.add("hidden");
      ssidSelect.classList.remove("has-spinner");
      console.error(err);
    });

  // Calculate recommended max current based on shunt resistance
  const shuntInput = document.getElementById("shunt_resistance");
  const maxCurrentInput = document.getElementById("max_current");
  const maxCurrentHint = document.getElementById("max_current_hint");

  function updateMaxCurrentHint() {
    const shunt = parseFloat(shuntInput.value);
    const maxCurrent = parseFloat(maxCurrentInput.value);

    if (!isNaN(shunt) && shunt > 0) {
      const maxAllowed = 0.0819 / shunt;
      maxCurrentHint.textContent = `Recommended: ${maxAllowed.toFixed(2)} A (based on shunt ${shunt} Ohm)`;
      maxCurrentHint.style.color = "var(--text-color)";

      if (!isNaN(maxCurrent) && maxCurrent > maxAllowed) {
        maxCurrentHint.textContent += " | WARNING: Current exceeds maximum!";
        maxCurrentHint.style.color = "#ff4444";
      }
    } else {
      maxCurrentHint.textContent = "";
    }
  }

  shuntInput.addEventListener("input", updateMaxCurrentHint);
  maxCurrentInput.addEventListener("input", updateMaxCurrentHint);
  updateMaxCurrentHint(); // Run on page load
});