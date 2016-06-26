<?php

$info["id"] = "player";
$info["version"] = "0.1-alpha";
$info["build"] = 2;

$info["en"]["name"] = "Player";
$info["en"]["desc"] = "This plugin allows to play <i>Sing to Jehovah—Piano Accompaniment</i>, <i>Sing to Jehovah—Orchestral Accompaniment</i> and <i>Sing to Jehovah—New Songs</i>.<br/><br/>Instruction:<br/>1. Download <i>iasn_X.mp3.zip</i>, <i>iasnm_X.mp3.zip</i> and <i>snnw_X.mp3.zip</i> from <a href=\"https://jw.org/\">jw.org</a><br/>2. Extract content of downloaded files into one directory (without subdirectories)<br/>3. Select the directory in plugin's settings tab<br/><br/><b>Note:</b> This plugin supports playing songs files of any language.";

$info["pl"]["name"] = "Odtwarzanie";
$info["pl"]["desc"] = "Ta wtyczka pozwala na odtwarzanie: <i>Śpiewajmy Jehowie — akompaniament fortepianowy</i>, <i>Śpiewajmy Jehowie — podkład orkiestrowy</i> i <i>Śpiewajmy Jehowie — nowe pieśni</i>.<br/><br/>Instrukcja:<br/>1. Pobierz <i>iasn_P.mp3.zip</i>, <i>iasnm_P.mp3.zip</i> i <i>snnw_P.mp3.zip</i> ze strony <a href=\"https://www.jw.org/pl/publikacje/muzyka-pie%C5%9Bni/\">jw.org</a><br/>2. Wypakuj zawartość pobranych plików do jednego folderu (bez podfolderów)<br/>3. Wybierz ten katalog w ustawieniach wtyczki";

file_put_contents("info.json", json_encode($info));

?>
