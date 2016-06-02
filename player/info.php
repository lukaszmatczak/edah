<?php

$info["id"] = "player";
$info["version"] = date("ymd");
$info["build"] = 1;

$info["en"]["name"] = "Player";
$info["en"]["desc"] = "Songs player";

$info["pl"]["name"] = "Odtwarzanie";
$info["pl"]["desc"] = "Odtwarzacz pieśni";

file_put_contents("info.json", json_encode($info));

?>
