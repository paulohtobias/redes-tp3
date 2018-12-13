<?php
	$ip_destino = $_POST['ip_destino'];
	$porta_destino = $_POST['porta_destino'];
	$taxa_corromper = $_POST['taxa_corromper'];
	$taxa_perder = $_POST['taxa_perder'];
	$taxa_atrasar = $_POST['taxa_atrasar'];
	$dado = $_POST['dado'];
	$size = $dado;

	$type = $_POST['type'];
	if($type == 0){
		$size = strlen($dado);
		system("sh msg.sh ".$size.' '.str_replace(" ", "_", $dado).' '.$taxa_corromper." ".$taxa_perder." ".$taxa_atrasar);
		
		echo file_get_contents("mensagem");
		echo ";".file_get_contents("mensagem_crua")."\n";
	}else{
		system("../main.out -q ".$padrao." -t ".$size.' -f "'.$dado.'" &'."../main.out -q -t ".$size." &");
	}
?>