<!DOCTYPE html>
<html>
<!-- font-family: Arial, sans-serif; -->
<head>
    <title>RozOS Dashboard</title>
    <style>
        body {
            background-color: #f0f0f0;
            font-family: Arial, sans-serif;
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;

        }
        .btn1 {
            background-color: #3498db;
            color: white;
            padding: 10px 25px;
            border-radius: 5px;
            border: 0;
            cursor: pointer;
            font-size: 16px;
        }
        .container {
            background: white;
            border-radius: 10px;
            padding: 50px;
            box-shadow: 0 0 150px rgba(0,0,0,0.1);
        }

        h1 {
            color: #2c3e50;
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px;
        }
        h2 {
            color: #34495e;
            margin-top: 25px;
            border-left: 4px solid #e74c3c;
            padding-left: 10px;
        }
        .warning {
            background: #fff3cd;
            border: 1px solid #ffeaa7;
            padding: 15px;
            border-radius: 5px;
            margin: auto 0;
        }
        .command-block {
            background: #2c3e50;
            color: white;
            padding: 15px;
            border-radius: 5px;
            font-family: 'Courier New', monospace;
            margin: 15px 0;
        }
        .feature-list {
            font-family: sans-serif;
            color: #34495e;
            margin: 20px 0;
        }
        p {
            font-family: Arial, sans-serif;
            font-size: 16px;
            color: #2c3e50;
        }
        .form-input {
            width: 50%;
            padding: 15px;
            margin: 20px 0;
            border: 2px solid #e0e0e0;
            border-radius: 10px;
            font-size: 16px;
            transition: all 0.3s ease;
            box-sizing: border-box;
        }
        h1t {
            display: block;
            font-size: 2em;
            color: #dfdfdf;
            font-weight: bold;
            margin: 0.67em 0;
            border-bottom: 3px solid #3498db;
            padding-bottom: 10px
            
        }
        .form-textarea {
            width: 100%;
            height: 120px;
            padding: 15px;
            margin: 20px 0;
            border: 2px solid #e0e0e0;
            border-radius: 10px;
            font-size: 16px;
            font-family: 'Courier New', monospace;
            resize: vertical;
            box-sizing: border-box;
        }


    </style>

    <script>
        function render() {
            const commandInput = document.getElementById("htmlCommandInput").value;
            const renderArea = document.getElementById("htmlRenderArea");
            renderArea.innerHTML = commandInput;

        }
        window.addEventListener('load', function() {
             // USUŃ WSZYSTKIE STYLE
             

             // Załaduj CSS z LocalStorage
             const zapisanyCSS = localStorage.getItem('cssname');
             if (zapisanyCSS) {
                const wszystkieStyle = document.querySelectorAll('style');
             wszystkieStyle.forEach(style => style.remove());
                 const style = document.createElement('style');
                 style.textContent = zapisanyCSS;
                 document.head.appendChild(style);
        }
        });
    </script>
</head>
<body>
<h2>Time: <?php echo date("d-m-y"); ?><h2>
<div class="container">
    <h1>RozOS Dashboard</h1>
    <h2>Welcome, Admin!</h2>
    <p>This is your dashboard where you can manage your RozOS platform.</p>
    <p></p>

    <h2>Html console</h2>
        <label>
            <textarea id="htmlCommandInput" class="form-textarea" placeholder="Enter HTML code here..." style="width: 700px; height: 300px;"></textarea>
            <button class="btn1" onclick="render()">Send</button>
        </label>
        
        <div class="command-block" id="htmlCommandOutput">
            <div id="htmlRenderArea"></div>
        </div>
    <p></p>
    <p></p>
    <div class="command-block">
        <code>Your admin commands:</code>
        <p></p>
        <code>window.location.href = "login.html";</code>
        <p></p>
        <code>localStorage.getItem("admin");</code>
        <p></p>
        <code>localStorage.removeItem("admin");</code>
        <p></p>
        <code>localStorage.setItem("admin");</code>
    </div>
    <div class="warning">
        <strong>Warning:</strong> Make sure to log out after your session to protect your admin access.
        <p></p>
        <button class="btn1" style="margin-left: 20px;" onclick="localStorage.removeItem('admin'); location.href='login.html'">Logout</button>
        <p></p>
        <button class="btn1" style="margin-left: 20px;" onclick="localStorage.removeItem('admin'); location.href='index.html'">Go to Main Page and Logout</button>
    </div>
</div>

<script>
    if (localStorage.getItem("admin") !== "true") {
        window.location.href = "login.html";
    }else {
        console.log("Admin access granted.");
    }
</script>
</body>
</html>