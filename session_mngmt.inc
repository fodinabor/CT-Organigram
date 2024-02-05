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

session_start();

$logged_in = false;
$error_message = "";
$asset_cache_time = date("mdH");

if(isset($_SESSION['csrf'])
	&& !((isset($_POST['csrf']) && hash_equals($_SESSION['csrf'], $_POST['csrf'])) 
	|| (isset($_GET['csrf']) && hash_equals($_SESSION['csrf'], $_GET['csrf'])))){ // CSRF protection
	$error_message = "Session couldn't be validated - logoff. Session konnte nicht validiert werden - ausgeloggt.";
	logoff();
}

if(isset($_GET['logoff'])){
	logoff();
}

if(isset($_POST['server'])){
    if(isset($_COOKIE['key'])){
        setcookie('key', $_COOKIE['key'], time() - 3600, "", $_SERVER['SERVER_NAME'], true, true); // clear cookie to make sure, that we get a clean key (prevent manipulated keys)
        unset($_COOKIE['key']);
    }
	
	$_POST['server'] = normalize_server($_POST['server']);
	
	if(isset($_POST['id']) && is_numeric($_POST['id']) && isset($_POST['token'])){
		if(CT_login($_POST['server'], $_POST['token'], $_POST['id'])){
			$_SESSION['csrf'] = bin2hex(random_bytes(32));
			$_SESSION['user'] = array('id' => encrypt_pw($_POST['id']), 'token' => encrypt_pw($_POST['token']), 'server' => encrypt_pw($_POST['server']));
			$logged_in = true;
		} else 
			$error_message = "Could not login using token & id. Anmeldung mit Token und ID nicht erfolgreich.";
	} else if(isset($_POST['email']) && isset($_POST['pw'])){
		if(CT_loginAuth($_POST['server'], $_POST['email'], $_POST['pw'])){
			$_SESSION['csrf'] = bin2hex(random_bytes(32));
			$_SESSION['user'] = array('email' => encrypt_pw($_POST['email']), 'pw' => encrypt_pw($_POST['pw']), 'server' => encrypt_pw($_POST['server']));
			$logged_in = true;
		} else 
			$error_message = "Could not login using email & password. Anmeldung mit E-Mail und Passwort nicht erfolgreich.";
	}
}

if(isset($_SESSION['user'])){
	$user = $_SESSION['user'];
	if(!$logged_in){
		if(isset($user['id']) && CT_login(decrypt_pw($user['server']), decrypt_pw($user['token']), decrypt_pw($user['id']))
			|| isset($user['email']) && CT_loginAuth(decrypt_pw($user['server']), decrypt_pw($user['email']), decrypt_pw($user['pw'])))
			$logged_in = true;
		else
			$error_message = "Could not login using cached information. Anmeldung zwischengespeicherten informationen nicht erfolgreich.";
	}
}

if(!$logged_in)
	build_login_form();

function encrypt_pw($pw){
	$key = getEncryptionKey();
	
	$cipher = "aes-256-cbc";
	if (in_array($cipher, openssl_get_cipher_methods()))
	{
		$ivlen = openssl_cipher_iv_length($cipher);
		$iv = openssl_random_pseudo_bytes($ivlen);
		$ciphertext_raw = openssl_encrypt($pw, $cipher, $key, $options=OPENSSL_RAW_DATA, $iv);
		$hmac = hash_hmac('sha256', $ciphertext_raw, $key, $as_binary=true);
		$ciphertext = base64_encode( $iv.$hmac.$ciphertext_raw );
		return $ciphertext;
	}
	logoff();
	die("Server not supported - missing cipher");
}

function decrypt_pw($pw){
	$key = getEncryptionKey();
	
	$cipher = "aes-256-cbc";
	if (in_array($cipher, openssl_get_cipher_methods()))
	{
		$c = base64_decode($pw);
		$ivlen = openssl_cipher_iv_length($cipher);
		$iv = substr($c, 0, $ivlen);
		$hmac = substr($c, $ivlen, $shalen=32);
		$ciphertext_raw = substr($c, $ivlen+$shalen);
		$plaintext = openssl_decrypt($ciphertext_raw, $cipher, $key, $options=OPENSSL_RAW_DATA, $iv);
		$calcmac = hash_hmac('sha256', $ciphertext_raw, $key, $as_binary=true);
		if (hash_equals($hmac, $calcmac))
		{
			return $plaintext;
		}
		logoff();
		die("Encrypted data changed. Not using it.");
	}
	logoff();
	die("Server not supported - missing cipher");
}

function normalize_server($server){
	if(strpos($server, "://") === false){
		$server = "https://" . $server;
	}
	
	if(strpos($server, "/?q=", strlen("https://")) === false) {
		$pos = strpos($server, "/", strlen("https://"));
		if($pos)
			$server = substr($server, 0, $pos);
		
		$server .= "/?q=";
	}
	return $server;
}

function logoff(){
	session_unset();
	session_destroy();
    if(isset($_COOKIE['key'])){
        setcookie('key', $_COOKIE['key'], time() - 3600, "", $_SERVER['SERVER_NAME'], true, true);
        unset($_COOKIE['key']);
    }
    if (isset($_COOKIE[session_name()])){
        setcookie(session_name(), $_COOKIE[session_name()], time()-3600, "/");
        unset($_COOKIE[session_name()]);
    }
}

function getEncryptionKey() {
	if(!$_SERVER['HTTPS'])
		die("Only HTTPS supported");
	if(!isset($_COOKIE['key']) || empty($_COOKIE['key'])){
        $_COOKIE['key'] = base64_encode(openssl_random_pseudo_bytes(32));
		setcookie('key', $_COOKIE['key'], 0, "", $_SERVER['SERVER_NAME'], true, true);
    }
	return base64_decode($_COOKIE['key']);
}

function build_login_form(){
	global $error_message;
	global $asset_cache_time;
	?>
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<title>CT Hierachie Visualisierung</title>
		<link rel="stylesheet" href="res/input.css?<?php echo $asset_cache_time; ?>">
	</head>
	<body>
		<?php
			if(!empty($error_message)){
				echo "<div class='error'><p>" . $error_message ."</p></div>";
			}
			include_once('config.inc');
		?>
		<h3>Please enter your user id &amp; token or E-Mail &amp; password:</h3>
		<p>If using 2FA you have to generate a token in CT and use the token login.</p>
		<p><i>Bitte gib entweder deine ChurchTools Benutzer ID und dein Login Token ein oder deine E-Mail Adresse und dein Passwort.<br>
		Wenn 2-Faktor Authentifizierung für deinen Account aktiviert ist, funktioniert nur ID &amp; Token login.</i></p>
		<div class="loginByToken">
		<form action="<?php echo $_SERVER['PHP_SELF']; ?>" method="POST">
			
			<label for="server"><b>CT Address</b></label>
			<input type="text" placeholder="Enter adress" name="server" required>
			
			<label for="id"><b>User ID</b></label>
			<input type="text" placeholder="Enter id" name="id" required>

			<label for="token"><b>Token</b></label>
			<input type="password" placeholder="Copy token" name="token" required />
			
			<select name="leader-roles[]" multiple class="hidden">
				<?php
					foreach ($group_leader_role_names as $groupRole) {
						echo "<option value='$groupRole' selected>$groupRole</option>";
					}
				?>
			</select>
			<select name="ma-roles[]" multiple class="hidden">
				<?php
					foreach ($group_members_role_names as $groupRole) {
						echo "<option value='$groupRole' selected>$groupRole</option>";
					}
				?>
			</select>
			<button type="submit">Login</button>
		</form>
		</div>
		
		<div class="loginByCreds">
		<form action="<?php echo $_SERVER['PHP_SELF']; ?>" method="POST">
			<label for="server"><b>CT Address</b></label>
			<input type="text" placeholder="Enter adress" name="server" required>
			
			<label for="email"><b>E-Mail</b></label>
			<input type="text" placeholder="Enter E-Mail" name="email" required>

			<label for="pw"><b>Password</b></label>
			<input type="password" placeholder="Enter Password" name="pw" required>
			

			<select name="leader-roles[]" multiple class="hidden">
				<?php
					foreach ($group_leader_role_names as $groupRole) {
						echo "<option value='$groupRole' selected>$groupRole</option>";
					}
				?>
			</select>
			<select name="ma-roles[]" multiple class="hidden">
				<?php
					foreach ($group_members_role_names as $groupRole) {
						echo "<option value='$groupRole' selected>$groupRole</option>";
					}
				?>
			</select>

			<button type="submit">Login (E-Mail &amp; PW)</button>
		</form>
		</div>
		<div style="float: left; padding-top: 10px;">
		<p>How to get token / id:<br>
		<i>Wie komme ich an Token / ID:</i><br>
		Select a person (usually you) in ChurchDB, then you'll find the id in the lower right corner. The link to aquire the login token is hidden in the Permissions. Possibly you need to ask your admin to give you the token. <b>You should be careful with the token as it is equivalent to your password!</b><br>
		<i>Wähle eine Person (normalerweise dich) in ChurchDB. Dann findest du rechts unten die ID. Der Link, um den Login Token zu erhalten, ist unter Berechtigungen versteckt. Ggf. beim Admin das Token anfragen. <b>Bitte sorgfältig mit dem Token umgehen, da er äquivalent zu deinem Passwort ist!</b></i><br>
		<img src="res/how-to-id.PNG" /><br><img src="res/how-to-berechtigungen.PNG" /><br><img src="res/how-to-token.PNG" />
		</p>
		<p><i>Privacy / Security:<br>
		JavaScript has to be activated for the website to work.
		No data is permanently stored on the server, though your ip address is logged to prevent abuse (logs are deleted regularily of course).
		The information you provide for login is not stored permanently on my server and only used to login to your ChurchTools instance. For convenience the email or the id, password or the token and the server address are <b>stored temporarily in encrpyted form</b>, so you don't have to login again and again.
		The information to decrypt this data is never stored on my server, but is stored <b>on your computer</b> for the duration of your session as a cookie. It will be deleted as soon, as you logoff or close your browser.<br>
		If you still have privacy concerns, you could just host this yourself. The code can be found on <a href="https://github.com/fodinabor/CT-Organigram">GitHub</a>.<br><br>
		JavaScript muss aktiviert sein.
		Keine der angegebenen Daten werden auf diesem Server permanent gespeichert. Nur die IP-Adresse mit der du gerade auf die Website zugreifst, um Missbrauch zu vermeiden (Logs werden natürlich regelmäßig gelöscht).
		Die Daten, die du angibst werden ausschließlich verwendet, um die Daten von deiner ChurchTools Instanz zu holen. Damit du nicht für jedes Mal Neuladen deine Daten neu angeben musst, werden die E-Mail Adresse bzw. die Nutzer ID, dein Passwort bzw. dein Token und die Server Adresse <b>in verschlüsselter Form temporär</b> zwischen gespeichert.
		Die nötigen Informationen, um deine Daten zu entschlüsseln werden nie auf diesem Server gespeichert, aber dafür als Cookie <b>auf deinem Computer</b> für die Dauer deiner Sitzung abgelegt. Das Cookie und die anderen Daten werden automatisch gelöscht, wenn du dich ausloggst oder den Browser schließt.<br>
		Solltest du immer noch Datenschutz Bedenken haben, kannst du dieses Organigram auch gerne auf deinem eigenen Server laufen lassen. Der Code steht auf <a href="https://github.com/fodinabor/CT-Organigram">GitHub</a> zur Verfügung.
		</i></p>
		<p><i>Disclaimer:<br>
		THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
		IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
		FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
		AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
		LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
		OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
		SOFTWARE.
		</i></p>
		</div>
	</body>
	</html>
	<?php
	exit;
}
