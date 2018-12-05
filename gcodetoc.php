<?php

$gcode_ary = explode("\n", file_get_contents("imperial_march2.gcode"));
//print_r($gcode_ary);
ob_start();

$count = 0;

echo "String imperial_march[{COUNT}] = {\n";

foreach($gcode_ary as $gcode) {
  if(trim($gcode)!='') {
    echo '  "'.$gcode.'",'."\n";
    $count++;
  }
}

echo '  ""'."\n};\n";
$count++;

$contents = str_replace("{COUNT}", $count, ob_get_contents());
ob_end_clean();

echo $contents;
