<?php
if (isset($_GET['getinfo'])) {
    $info = file_get_contents('info.json');

    if ($info) {
        echo $info;
    } else {
        echo "No info found.";
    }
} else
{
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
                        <h1 class="card-title text-center mb-5 fw-light fs-5">Firmware Updater Web Tool</h5>
    <?php


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
                <p>File downloaded and info saved</p>
    <?php


    } 
    else
    {
        ?>

                        <form method="POST">
                        <div class="form-floating mb-3">
                            <input type="number" class="form-control" name="minor" placeholder="1">
                            <label for="floatingInput">Version Minor</label>
                        </div>
                        <div class="form-floating mb-3">
                            <input type="number" class="form-control" name="major" placeholder="20">
                            <label for="floatingInput">Version Major</label>
                        </div>
                        <div class="form-floating mb-3">
                            <input type="link" class="form-control" name="file_link" placeholder="https://raw.github.com/app.bin">
                            <label for="floatingLink">File Link</label>
                        </div>
                        <div class="d-grid">
                            <button class="btn btn-primary btn-login text-uppercase fw-bold" type="submit">submit</button>
                        </div>
                        <hr class="my-4">

                        <div class="d-grid">
                        <a href="https://github.com/hmda77/STM32F429ZIT6_OTA" target="_blank" class="btn btn-login text-uppercase fw-bold github-link">
                        <i class ="fa fa-github me-2"></i>
                            </a>
                            </button>
                        </div>
                        </form>
        <?php
    }
    ?>
                    </div>
                    </div>
                </div>
                </div>
            </div>
        </body>
        <?php
}
?>
