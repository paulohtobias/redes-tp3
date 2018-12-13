<?php
	$rtt_ms = $_POST['rtt_ms'];
	$taxa_corromper = $_POST['taxa_corromper'];
	$taxa_perder = $_POST['taxa_perder'];
	$taxa_atrasar = $_POST['taxa_atrasar'];
	$dado = $_POST['dado'];
	$size = $dado;

	$type = $_POST['type'];
	if($type == 0){
		$size = strlen($dado);
		system("sh msg.sh ".$size.' '.'"'.$dado.'"'.' '.$taxa_corromper." ".$taxa_perder." ".$taxa_atrasar." ".$rtt_ms);
		
		echo file_get_contents("mensagem");
		echo ";".file_get_contents("mensagem_crua")."\n";
	}
?>