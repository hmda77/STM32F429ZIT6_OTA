<?php
if (isset($_GET['newfw'])) {
    if ($_SERVER['REQUEST_METHOD'] === 'POST') {
        $major = $_POST['major'];
        $minor = $_POST['minor'];
        $file_link = $_POST['file_link'];

        // Validate and sanitize input here if needed

        $filename = "$major-$minor.bin";
        $file_content = file_get_contents($file_link);
        file_put_contents($filename, $file_content);

        $file_info = array(
            'fw_version' => array(
                'minor' => (int)$minor,
                'major' => (int)$major
            ),
            'fw_crc' => hash_file('crc32b', $filename),
            'fw_size' => filesize($filename),
            'fw_link' => $file_link
        );

        // Use JSON_UNESCAPED_SLASHES option to prevent escaping slashes
        file_put_contents('info.json', json_encode($file_info, JSON_PRETTY_PRINT | JSON_UNESCAPED_SLASHES));

        ?>

        <html>
        <header>
        <link rel="stylesheet" href="css/font-awesome.min.css">
        <link rel="stylesheet" href="path/to/font-awesome/css/font-awesome.min.css">
                <link rel="stylesheet" href="style.css">
        </header>
        <body>
            <div class="container">
                <h1>FW Update Information</h1>
                <hr style="width: 100%;">
                <p>File downloaded and info saved</p>
                <a href="https://github.com/hmda77/STM32F429ZIT6_OTA" target="_blank" class="github-link">
                    <i class ="fa fa-github"></i>
                </a>
            </div>
        </body>
        </html>
        <?php

    } else {
        ?>
        <html>
        <header>
                <link rel="stylesheet" href="css/font-awesome.min.css">
                <link rel="stylesheet" href="css/style.css">
        </header>
        <body>
            <div class="container">
                <h1>FW Update Information</h1>
                <hr style="width: 100%;">
                <form method="post">
                    Major: <input type="text" name="major"><br>
                    Minor: <input type="text" name="minor"><br>
                    File Link: <input type="text" name="file_link"><br>
                    <input type="submit" value="Submit">
                </form>
                <a href="https://github.com/hmda77/STM32F429ZIT6_OTA" target="_blank" class="github-link">
                    <i class ="fa fa-github"></i>
                </a>
            </div>
        </body>
        </html>
        <?php
    }
} elseif (isset($_GET['getinfo'])) {
    $info = file_get_contents('info.json');

    if ($info) {
        echo $info;
    } else {
        echo "No info found.";
    }
}
?>