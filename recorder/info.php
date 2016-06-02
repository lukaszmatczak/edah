<?php

$info["id"] = "player";
$info["version"] = date("ymd");
$info["build"] = 1;

$info["en"]["name"] = "Recorder";
$info["en"]["desc"] = "This plugin can create MP3 file from audio signal connected to PC";

$info["pl"]["name"] = "Nagrywanie";
$info["pl"]["desc"] = "Wtyczka, która pozwala nagrywać do pliku MP3 sygnał audio podłączony do komputera";

file_put_contents("info.json", json_encode($info));

?>
