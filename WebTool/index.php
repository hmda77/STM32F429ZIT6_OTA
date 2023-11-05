<?php
if (isset($_GET['getinfo'])) {
    $info = file_get_contents('info.json');

    if ($info) {
        echo $info;
    } else {
        echo "No info found.";
    }
}
else{

    ?>

    <html>
        <header>
        
        <!-- <link rel="stylesheet" href="css/all.min.css"> -->
        <!-- Bootstrap CSS -->
        <link rel="stylesheet" href="css/bootstrap.min.css">
        <!-- JQuery -->
        <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.6.0/jquery.min.js"></script>
        <!-- Bootstrap JS -->
        <script src="js/bootstrap.bundle.min.js"></script>

        <link rel="stylesheet" href="css/font-awesome.min.css">
        <link rel="stylesheet" href="css/style.css">

        </header>
        <body>
            <div class="container">
                <div class="row">
                <div class="col-sm-9 col-md-7 col-lg-5 mx-auto">
                    <div class="card border-0 shadow rounded-3 my-5">
                    <div class="card-body p-4 p-sm-5">
                        <h1 class="card-title text-center mb-5 fw-light fs-2">Firmware Updater Web Tool</h5>
    <?php


    if ($_SERVER['REQUEST_METHOD'] === 'POST') {
        
        // set a password here
        $password = "stm32update";
        $version = explode(".", $_POST['version']);
        $major = $version[0];
        $minor = $version[1];
        $file_link = $_POST['file_link'];
        $received_password = $_POST['password'];

        // Validate and sanitize input here if needed
        if($received_password == $password)
        {
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
        <h5 class ="text-center fw-light fs-5 mb-3">File downloaded and info saved</h5>
        <ul>
            <li>Firmware Version: <?php echo $major.".".$minor ?> </li>
            <li>Firmware CRC: <?php echo "0x".strtoupper($file_info['fw_crc']) ?> </li>
            <li>Firmware Size: <?php echo round(((int)$file_info['fw_size'])/1024)." KB (".$file_info['fw_size']." B)" ?> </li>
            <li>Firmware Link: <a href="<?php echo $file_info['fw_link'] ?>" target="_blank"><?php echo $file_info['fw_link'] ?></a></li>
        </ul>
        <?php
        }
        else
        {
            ?>
            <h5 class ="text-center fs-5 mb-3" >Access denied!</h5>
            <?php
        }


    } 
    else
    {
        ?>

                        <form method="POST">
                        <div class="form-floating mb-3">
                            <input type="number" step=".0001" class="form-control" name="version" placeholder="1">
                            <label for="floatingInput">Version Minor</label>
                        </div>
                        <div class="form-floating mb-3">
                            <input type="url" class="form-control" name="file_link" placeholder="https://raw.github.com/app.bin">
                            <label for="floatingLink">File Link</label>
                        </div>
                        <div class="form-floating mb-3">
                            <input type="password" class="form-control" name="password" placeholder="Password">
                            <label for="floatingPassword">Password</label>
                        </div>
                        <div class="d-grid">
                            <button class="btn btn-primary btn-login text-uppercase fw-bold" type="submit">submit</button>
                        </div>
                        </form>
        <?php
    }
    ?>
                        <hr class="my-4">

                        <div class="d-grid">
                        <a href="https://github.com/hmda77/STM32F429ZIT6_OTA" target="_blank" class="btn btn-login text-uppercase fw-bold github-link">
                        <i class ="fa fa-github me-2"></i>
                            </a>
                            </button>
                        </div>
                    </div>
                    </div>
                </div>
                </div>
            </div>
        </body>
        <?php
}
?>