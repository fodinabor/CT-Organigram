<?php
/*
MIT License

Copyright (c) Joachim Meyer 2018 

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
require_once("cthelper.inc");
include_once('session_mngmt.inc');

$active_domain = $_SESSION['user']['server'];
?>
<!DOCTYPE html>
<html lang="en">
<head>
<?php
	echo '<link rel="stylesheet" href="res/jquery.orgchart.css?' . time() .'">';
?>
<style>
    .orgchart {
        background-image: none;
    }
    .orgchart .node .title {
        background-color: #aaa;
        color: #00039a;
    }
    @media print {
        .orgchart .node .title {
            background-color: #eee;
            border: 1px solid black;
            border-width: 1px;
            color: #00039a;
            -webkit-print-color-adjust: exact;
        }
    }
    .chbox {
        display: inline-flex;
    }
</style>
<script type="text/javascript">
(print_window = function () {
	var novoForm = window.open('', 'Print', 'status=no,toolbar=no,scrollbars=yes,titlebar=no,menubar=no,resizable=yes,width=600,height=600,directories=no,location=no');
	novoForm.document.write($(document.documentElement).html());
	novoForm.document.getElementById('#print').remove();
	novoForm.document.getElementById('#logout').remove();
});
</script>
<!--script type="text/javascript" src="res/jquery.min.js"></script>-->
<script
  src="https://code.jquery.com/jquery-3.3.1.min.js"
  integrity="sha256-FgpCb/KJQlLNfOu91ta32o/NMZxltwRo8QtmkMRdAu8="
  crossorigin="anonymous"></script>
<?php
	echo '<script type="text/javascript" src="res/jquery.orgchart.js?t'.time().'"></script>';
?>
</head>
<body>
<a id="#print" href="javascript:void(0);" rel="nofollow" onclick="print_window();" title="Print">Print</a>
<a id="#logout" href="<?php echo $_SERVER['PHP_SELF'] . "?logoff=1"; ?>" >Logout</a>
<?php

function exceptionHandler($ex){
	global $active_domain;
	echo "An error occured.";
	if(isset($active_domain))
		CT_logout($active_domain);
}

set_exception_handler('exceptionHandler');

function printElement($element, $hierachy, $letzte = false){
    if(!empty($element["name"])){
		echo "<li " .
		(empty($element["leader"]) ? "" : "data-leader='" . implode("<br>",$element["leader"]) . "' ") .
		(empty($element["ma"])     ? "" : "data-ma='" . implode("<br>",$element["ma"]) . "'") .
		">".$element["name"];
	}
    if($element["children"] !== null && !$letzte){
		if(!empty($element["name"]))
			echo "<ul>";
        foreach($element["children"] as $child){
            printElement($hierachy[$child], $hierachy, $element['type'] === "7");
        }
		if(!empty($element["name"]))
			echo "</ul>";
    }
	if(!empty($element["name"]))
		echo "</li>";
}

$masterData = CT_getCTAuthMasterData($active_domain);
if($masterData->status != "success"){
    CT_logout($active_domain);
    die("Error fetching data");
}

$person_data = CT_getAllPersonData($active_domain);
if($person_data->status != "success"){
    CT_logout($active_domain);
    die("Error fetching person data");
}

CT_logout($active_domain);

$person_data = $person_data->data;
$masterData = $masterData->data;
$groups = $masterData->churchauth->group;
$level = intval(isset($_GET['level'])? $_GET['level'] : 3);

$gtmss = get_object_vars($masterData->churchauth->grouptypeMemberstatus);
$gmss = get_object_vars($masterData->churchauth->groupMemberstatus);

$groupTypes = array();
foreach($grouptype as $id => $data){
	$groupTypes[$id] = $data->bezeichnung;
}

$hierachy = array();

foreach($groups as $id => $data){	
	if($data->versteckt_yn === "0"/* && $data->groupstatus_id === "1"*/){
		$hierachy[$id] = array("name" => $data->bezeichnung, "type" => $groupTypes[$data->gruppentyp_id], "leader" => array(), "ma" => array(), "children" => $data->childs, "parents" => $data->parents);
	} else {
		$hierachy[$id] = array("name" => "", "type" => $groupTypes[$data->gruppentyp_id], "leader" => array(), "ma" => array(), "children" => $data->childs, "parents" => $data->parents);
	}
}

foreach($person_data as $p_id => $person) {
	if(!isset($person->groupmembers))
		continue;
	foreach($person->groupmembers as $gm_id => $gm){
		if(empty($hierachy[$gm_id]['name']))
			continue;
		
		$gms = $gtmss[$gm->groupmemberstatus_id];
		if($gms->bezeichnung == "Leiter"){
			$hierachy[$gm_id]['leader'][] = $person->vorname . " " . $person->name;
		} else if($gms->bezeichnung == "Mitarbeiter"){
			$hierachy[$gm_id]['ma'][] = $person->vorname . " " . $person->name;
		} else if($gms->bezeichnung == "Ansprechpartner GL"){
			$hierachy[$gm_id]['leader'][] = $person->vorname . " " . $person->name;
		} else if($gms->bezeichnung == "Arbeitskreisleiter"){
			$hierachy[$gm_id]['leader'][] = $person->vorname . " " . $person->name;
		}
		
		// print_r($gms->group_id);
		// echo "<br>";
		// print_r($hierachy[$gm_id]);
		// echo "<br>";echo "<br>";
	}
}

echo "<div id='graphs'>";
foreach($hierachy as $id => $data){
    if($data["parents"] === null){
        echo "<div id='chart-container{$id}' class='chbox'><ul id='hierachy{$id}' hidden>";
        printElement($data, $hierachy, $data['type'] === "7");
        echo "</ul>
        <script type='text/javascript'>
         $(function() {
          $('#chart-container{$id}').orgchart({
            'data' : $('#hierachy{$id}'),
            'verticalLevel': {$level},
			'nodeContent': 'leader',
			'nodeContentSecond': 'ma'
          });
         });
        </script>
        </div>";
    }
}
echo "</div>";
?>



</body>
</html>


<?php
// echo json_encode($hierachy);

