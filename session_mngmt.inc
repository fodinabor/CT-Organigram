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
			$_SESSION['user'] = array('token' => $_POST['token'], 'id' => encrypt_pw($_POST['id']), 'server' => $_POST['server']);
			$logged_in = true;
		}
	} else if(isset($_POST['email']) && isset($_POST['pw'])){
		if(CT_loginAuth($_POST['server'], $_POST['email'], $_POST['pw'])){
			$_SESSION['user'] = array('email' => $_POST['email'], 'pw' => encrypt_pw($_POST['pw']), 'server' => $_POST['server']);
			$logged_in = true;
		}
	}
}

if(isset($_SESSION['user'])){
	$user = $_SESSION['user'];
	if(!$logged_in){
		if(isset($user['token']) && CT_login($user['server'], $user['token'], decrypt_pw($user['id'])) 
			|| isset($user['email']) && CT_loginAuth($user['server'], $user['email'], decrypt_pw($user['pw'])))
			$logged_in = true;
	}
}

if(!$logged_in)
	build_login_form();

function encrypt_pw($pw){	
	if(!file_exists('.seesion_key')){
		file_put_contents('.session_key', base64_encode(openssl_random_pseudo_bytes(2048)));
	}

	$key = base64_decode(file_get_contents('.seesion_key'));
	
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
	if(!file_exists('.seesion_key')){
		file_put_contents('.session_key', base64_encode(openssl_random_pseudo_bytes(2048)));
	}

	$key = base64_decode(file_get_contents('.seesion_key'));
	
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
}

function build_login_form(){
	?>
	<!DOCTYPE html>
	<html lang="en">
	<head>
		<title>CT Hierachie Visualisierung</title>
	</head>
	<body>
		<h3>Please enter id &amp; token or E-Mail &amp; password:</h3>
		<p>If using 2FA you have to generate a token in CT and use the token login.</p>
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
	</body>
	</html>
	<?php
	exit;
}
