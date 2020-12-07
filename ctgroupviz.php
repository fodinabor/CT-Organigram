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

$asset_cache_time = date("mdH");

$active_domain = decrypt_pw($_SESSION['user']['server']);

function ParameterOrDefault($param_name, $default = "")
{
  if (isset($_GET[$param_name]))
    return $_GET[$param_name];
  else if (isset($_POST[$param_name]))
    return $_POST[$param_name];
  return $default;
}

$level = intval(ParameterOrDefault('level', 3));

$group_leader_role_names = ParameterOrDefault('leader-roles', array());
$group_members_role_names = ParameterOrDefault('ma-roles', array());

?>
<!DOCTYPE html>
<html lang="en">

<head>
  <title>CT Hierachie Visualisierung</title>
  <?php
  echo '<link rel="stylesheet" href="res/jquery.orgchart.css?' . $asset_cache_time . '">';
  echo '<link rel="stylesheet" href="res/input.css?' . $asset_cache_time . '">';
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

      .orgchart .inactive .title {
        background-color: #FFF;
        color: #00020a;
      }

      .no-print {
        display: none;
      }
    }

    .chbox {
      display: inline-flex;
    }

    .error {
      background-color: #d30303;
      color: white;
      padding: 14px 20px;
      margin: 8px 0;
      border: none;
    }

    .orgchart .inactive .title {
      background-color: #ccc;
      color: #777;
    }

    .orgchart .inactive .content {
      background-color: #d2e4d2;
      color: #777;
    }

    .orgchart .inactive .content2 {
      color: #777;
    }

    .orgchart .other .title {
      background-color: #bbb;
      color: #00039a;
    }

    .orgchart .other .content {
      background-color: #d2e4d2;
      color: #777;
    }

    .orgchart .other .content2 {
      color: #777;
    }

    .orgchart .content:empty {
      display: none;
    }

    .orgchart .content2:empty {
      display: none;
    }
  </style>
  <script src="https://code.jquery.com/jquery-3.3.1.min.js" integrity="sha256-FgpCb/KJQlLNfOu91ta32o/NMZxltwRo8QtmkMRdAu8=" crossorigin="anonymous"></script>
  <?php
  echo '<script type="text/javascript" src="res/jquery.orgchart.js?t' . $asset_cache_time . '"></script>';
  ?>
</head>

<body>
  <?php

  function exceptionHandler($ex)
  {
    global $active_domain;
    echo "An error occured." . $ex;
    if (isset($active_domain))
      CT_logout($active_domain);
  }

  function showErrorAndExit($err)
  {
    echo "<div class='error'><p>" . $err . "</p></div>";
    exit();
  }

  set_exception_handler('exceptionHandler');

  function printElement($element, $hierachy)
  {
    if (!empty($element["name"])) {
      echo "<li " .
        (empty($element["leader"]) ? "" : "data-leader='" . implode("<br>", $element["leader"]) . "' ") .
        (empty($element["ma"])     ? "" : "data-ma='" . implode("<br>", $element["ma"]) . "'") .
        ("data-class='" . ($element["status"] === "1" ? "active" : ($element["status"] === "3" ? "inactive" : "other")) . "'") .
        ">" . $element["name"];
    }
    if ($element["children"] !== null) {
      if (!empty($element["name"]))
        echo "<ul>";
      foreach ($element["children"] as $child) {
        printElement($hierachy[$child], $hierachy);
      }
      if (!empty($element["name"]))
        echo "</ul>";
    }
    if (!empty($element["name"]))
      echo "</li>";
  }

  $groupRoles = CT_getAllGroupRoles($active_domain);
  $groupRoleNames = array();
  foreach ($groupRoles as $groupRole) {
    array_push($groupRoleNames, $groupRole->name);
  }
  $groupRoleNamesUnique = array_unique($groupRoleNames);

  ?>
  <div class="orgchart-options no-print">
    <form action="<?php echo $_SERVER['PHP_SELF']; ?>" method="POST">
      <label for="level">Vertical Level</label>
      <input type="number" min="0" step="1" name="level" value="<?php echo $level; ?>" required><br>
      <label for="leader-roles[]">Group leader roles (un-/select multiple with CRTL / CMD)</label>
      <select name="leader-roles[]" multiple>
        <?php
        foreach ($groupRoleNamesUnique as $groupRole) {
          $attr = in_array($groupRole, $group_leader_role_names) ? "selected" : "";
          echo "<option value='$groupRole' $attr>$groupRole</option>";
        }
        ?>
      </select><br>
      <label for="ma-roles[]">Group member roles (un-/select multiple with CRTL / CMD)</label>
      <select name="ma-roles[]" multiple>
        <?php
        foreach ($groupRoleNamesUnique as $groupRole) {
          $attr = in_array($groupRole, $group_members_role_names) ? "selected" : "";
          echo "<option value='$groupRole' $attr>$groupRole</option>";
        }
        ?>
      </select>
      <input type="hidden" name="csrf" value="<?php echo $_SESSION['csrf']; ?>" required>
      <button type="submit">Set</button>
    </form>
    <br>
    <a href="javascript:void(0);" rel="nofollow" onclick="window.print();" title="Print">
      <div class="button">Print</div>
    </a>
    <a href="<?php echo $_SERVER['PHP_SELF'] . "?csrf=" . $_SESSION['csrf'] . "&logoff=1"; ?>">
      <div class="button">Logout</div>
    </a>
  </div>
  <?php

  $masterData = CT_getCTDbMasterData($active_domain);
  if ($masterData->status != "success") {
    CT_logout($active_domain);
    showErrorAndExit("Error fetching data");
  }

  $person_data = CT_getAllPersonData($active_domain);
  if ($person_data->status != "success") {
    CT_logout($active_domain);
    showErrorAndExit("Error fetching person data");
  }

  CT_logout($active_domain);

  $person_data = $person_data->data;
  $masterData = $masterData->data;
  $groups = $masterData->groups;

  $gtmss = get_object_vars($masterData->grouptypeMemberstatus);
  $gmss = get_object_vars($masterData->groupMemberstatus);

  $groupTypes = array();
  foreach ($masterData->grouptypes as $id => $data) {
    $groupTypes[$id] = $data->bezeichnung;
  }

  $hierachy = array();

  foreach ($groups as $id => $data) {
    if ($data->versteckt_yn === "0"/* && $data->groupstatus_id === "1"*/) {
      $hierachy[$id] = array("name" => $data->bezeichnung, "type" => $groupTypes[$data->gruppentyp_id], "leader" => array(), "ma" => array(), "children" => $data->childs, "parents" => $data->parents, "status" => $data->groupstatus_id);
    } else {
      $hierachy[$id] = array("name" => "", "type" => $groupTypes[$data->gruppentyp_id], "leader" => array(), "ma" => array(), "children" => $data->childs, "parents" => $data->parents, "status" => $data->groupstatus_id);
    }
  }

  foreach ($person_data as $p_id => $person) {
    if (!isset($person->groupmembers))
      continue;
    foreach ($person->groupmembers as $gm_id => $gm) {
      if (empty($hierachy[$gm_id]['name']))
        continue;

      $gms = $gtmss[$gm->groupmemberstatus_id];
      if (in_array($gms->bezeichnung, $group_leader_role_names)) {
        $hierachy[$gm_id]['leader'][] = $person->vorname . " " . $person->name;
      } else if (in_array($gms->bezeichnung, $group_members_role_names)) {
        $hierachy[$gm_id]['ma'][] = $person->vorname . " " . $person->name;
      }
    }
  }

  echo "<div id='graphs'>";
  foreach ($hierachy as $id => $data) {
    if ($data["parents"] === null) {
      echo "<div id='chart-container{$id}' class='chbox'><ul id='hierachy{$id}' hidden>";
      printElement($data, $hierachy);
      echo "</ul>
        <script type='text/javascript'>
         $(function() {
          $('#chart-container{$id}').orgchart({
            'data' : $('#hierachy{$id}'),
            'verticalLevel': {$level},
			'nodeContent': 'leader',
			'nodeContentSecond': 'ma',
			'createNode': function (node, data) {
				node.addClass(data.class);
				}
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