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
include_once('config.inc');

$active_domain = decrypt_pw($_SESSION['user']['server']);

$level = intval(isset($_GET['level'])? $_GET['level'] : (isset($_POST['level']) ? $_POST['level'] : 3));
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
	var printWindow = window.open('', 'Print', 'status=no,toolbar=no,scrollbars=yes,titlebar=no,menubar=no,resizable=yes,width=600,height=600,directories=no,location=no');
	var doc = $(document.documentElement).clone();
	doc.find('#print').remove();
	doc.find('#logout').remove();
	doc.find('#level_form').remove();
	printWindow.document.write(doc.html());
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
<form id="level_form" action="<?php echo $_SERVER['PHP_SELF']; ?>" method="POST">
	<label for="level">Vertical Level</label>
	<input type="number" min="0" step="1" name="level" value="<?php echo $level; ?>" required>
	<input type="hidden" name="csrf" value="<?php echo $_SESSION['csrf']; ?>" required>
	<button type="submit">Set</button>
</form>
<br>
<a id="print" href="javascript:void(0);" rel="nofollow" onclick="print_window();" title="Print">Print</a>
<a id="logout" href="<?php echo $_SERVER['PHP_SELF'] . "?csrf=" . $_SESSION['csrf'] . "&logoff=1"; ?>" >Logout</a>
<?php

function exceptionHandler($ex){
	global $active_domain;
	echo "An error occured.";
	if(isset($active_domain))
		CT_logout($active_domain);
}

set_exception_handler('exceptionHandler');

function printElement($element, $hierachy){
    if(!empty($element["name"])){
		echo "<li " .
		(empty($element["leader"]) ? "" : "data-leader='" . implode("<br>",$element["leader"]) . "' ") .
		(empty($element["ma"])     ? "" : "data-ma='" . implode("<br>",$element["ma"]) . "'") .
		">".$element["name"];
	}
    if($element["children"] !== null){
		if(!empty($element["name"]))
			echo "<ul>";
        foreach($element["children"] as $child){
            printElement($hierachy[$child], $hierachy);
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
		if(in_array($gms->bezeichnung, $group_leader_role_names)){
			$hierachy[$gm_id]['leader'][] = $person->vorname . " " . $person->name;
		} else if(in_array($gms->bezeichnung, $group_members_role_names)){
			$hierachy[$gm_id]['ma'][] = $person->vorname . " " . $person->name;
		}
	}
}

echo "<div id='graphs'>";
foreach($hierachy as $id => $data){
    if($data["parents"] === null){
        echo "<div id='chart-container{$id}' class='chbox'><ul id='hierachy{$id}' hidden>";
        printElement($data, $hierachy);
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