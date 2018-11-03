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

if(isset($_GET['logoff'])){
	logoff();
}

if(isset($_POST['server'])){
	$_POST['server'] = normalize_server($_POST['server']);
	
	if(isset($_POST['id']) && is_numeric($_POST['id']) && isset($_POST['token'])){
		if(CT_login($_POST['server'], $_POST['token'], $_POST['id'])){
			setcookie('token', encrypt_pw($_POST['token']));
			$_SESSION['user'] = array('id' => $_POST['id'], 'server' => $_POST['server']);
			$logged_in = true;
		}
	} else if(isset($_POST['email']) && isset($_POST['pw'])){
		if(CT_loginAuth($_POST['server'], $_POST['email'], $_POST['pw'])){
			setcookie('key', encrypt_pw($_POST['pw']));
			$_SESSION['user'] = array('email' => $_POST['email'], 'server' => $_POST['server']);
			$logged_in = true;
		}
	}
}

if(isset($_SESSION['user'])){
	$user = $_SESSION['user'];
	if(!$logged_in){
		if(isset($user['id']) && CT_login($user['server'], decrypt_pw($_COOKIE['token']), $user['id']) 
			|| isset($user['email']) && CT_loginAuth($user['server'], $user['email'], decrypt_pw($_COOKIE['key'])))
			$logged_in = true;
	}
}

if(!$logged_in)
	build_login_form();

function encrypt_pw($pw){	
	if(!file_exists('.session_key')){
		file_put_contents('.session_key', base64_encode(openssl_random_pseudo_bytes(2048)));
	}

	$key = base64_decode(file_get_contents('.session_key'));
	
	$cipher = "aes-256-cbc";
	if (in_array($cipher, openssl_get_cipher_methods()))
	{
		$ivlen = openssl_cipher_iv_length($cipher);
		$iv = openssl_random_pseudo_bytes($ivlen);
		$ciphertext = openssl_encrypt($pw, $cipher, $key, $options=0, $iv);
		$ciphertext .= ":" . $iv;
		return base64_encode($ciphertext);
	}
	die("Server not supported - missing cipher");
}

function decrypt_pw($pw){	
	if(!file_exists('.session_key')){
		file_put_contents('.session_key', base64_encode(openssl_random_pseudo_bytes(2048)));
	}

	$key = base64_decode(file_get_contents('.session_key'));
	
	$cipher = "aes-256-cbc";
	if (in_array($cipher, openssl_get_cipher_methods()))
	{
		$pw_expl = explode(':', base64_decode($pw));
		return openssl_decrypt($pw_expl[0], $cipher, $key, $options=0, $pw_expl[1]);
	}
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
	session_destroy();
	$_SESSION = array();
	setcookie("token", "", time() - 3600);
	setcookie("key", "", time() - 3600);
}

function build_login_form(){
	?>
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<title>CT Hierachie Visualisierung</title>
		<style type="text/css">
			/* Full-width inputs */
			input[type=text], input[type=password] {
				width: 100%;
				padding: 12px 20px;
				margin: 8px 0;
				display: inline-block;
				border: 1px solid #ccc;
				box-sizing: border-box;
			}

			/* Set a style for all buttons */
			button {
				background-color: #4CAF50;
				color: white;
				padding: 14px 20px;
				margin: 8px 0;
				border: none;
				cursor: pointer;
				width: 100%;
			}

			/* Add a hover effect for buttons */
			button:hover {
				opacity: 0.8;
			}
			
			.loginByToken {
				max-width: 400px;
				float: left;
			}
			
			.loginByCreds {
				max-width: 400px;
				float: right;
			}
		</style>
	</head>
	<body>
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
			<input type="text" placeholder="Copy token" name="token" required>

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
		No data is permanently stored on the server, though your ip address is logged to prevent abuse.
		The information you provide for login is not stored on my server and only used to login to your ChurchTools instance. For convenience the email or the id and the server address are stored temporarily, so you don't have to login again and again.
		Your password / token is never stored on my server, but is stored <b>on your computer in an encrpyted form</b> for the duration of your session as a cookie. It will be deleted as soon, as you logoff or close your browser.<br>
		If you still have privacy concerns, you could just host this yourself. The code can be found on <a href="https://github.com/fodinabor/CT-Organigram">GitHub</a>.<br><br>
		Keine der angegebenen Daten werden auf diesem Server permanent gespeichert. Nur die IP-Adresse mit der du gerade auf die Website zugreifst, um Missbrauch zu vermeiden.
		Die Daten, die du angibst werden ausschließlich verwendet, um die Daten von deiner ChurchTools Instanz zu holen. Damit du nicht für jedes Mal Neuladen deine Daten neu angeben musst, werden die E-Mail Adresse bzw. die Nutzer ID und die Server Adresse zwischen gespeichert.
		Dein Passwort / Token wird nie auf diesem Server gespeichert, aber dafür als Cookie <b>auf deinem Computer in verschlüsselter Form</b> für die Dauer deiner Sitzung. Das Cookie und die anderen Daten werden automatisch gelöscht, wenn du dich ausloggst oder den Browser schließt.<br>
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
