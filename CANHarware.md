# Comment initilaiser une interface CAN sur un uC STM32?

## Description

1. Le STM32F0 à deux interfaces de CAN (CAN1 et CAN2), donc pour l'activer il faut passer la fonction : 
`__HAL_RCC_CAN1_CLK_ENABLE()` ou `__HAL_RCC_CAN2_CLK_ENABLE()`; une seul fois dans le code en fonction du canal choisi.
2. Configure les broche du STM32 qui seront utilisé dans notre application avec la structure `GPIO_InitTypeDef` de HAL. Et le données à la fonction qui configure le GPIO du stm32 `HAL_GPIO_Init(GPIOB, &itd)` en choisiçant la broche B ou A.
3. Initialiser notre interface CAN grâce à la structure que nous avons créer pour stocker les information sur le bus CAN.
`can_data_t *hcan`
  
   ``c
    hcan->instance   = instance;
	hcan->brp        = 6;
	hcan->phase_seg1 = 13;
	hcan->phase_seg2 = 2;
	hcan->sjw        = 1;
    ``

4. Configuration du timming du bus CAN (`vitesse de communication`, `nombre de quanta`, etc.)
    - Vérification des  valeur données.
    - Enregistrement dans la structure `hcan` si tout est ok.

``c
if ( (brp>0) && (brp<=1024)
	  && (phase_seg1>0) && (phase_seg1<=16)
	  && (phase_seg2>0) && (phase_seg2<=8)
	  && (sjw>0) && (sjw<=4)
	) {
		hcan->brp = brp & 0x3FF;
		hcan->phase_seg1 = phase_seg1;
		hcan->phase_seg2 = phase_seg2;
		hcan->sjw = sjw;
		return true;
	} else {
		return false;
}
``
* `brp` 10 bits de large (0–1023), donc 1024 valeurs possibles
* `phase_seg1` codé sur 4 bits
* `phase_seg2` codé sur 3 bits
* `Synchro nization Jump Width (sjw)` codé sur 2 bits

> Ces bornes viennent directement du Reference Manual du STM32 (registre BTR du périphérique CAN)

5. Activer et contrôler complètement le contrôleur du bus CAN du stm32 en, en utilisant les paramètres déjà stockés dans ta `structure can_data_t (brp, phase_seg1, etc.)`.
    - La remise à zéro du périphérique CAN
    - Le mode de fonctionnement (normal, boucle interne, écoute seule, one-shot…)
    - Le timing des bits (BTR)
    - La configuration des filtres de réception
    - L’éventuelle sortie du mode veille

5. 1. Préparation du registre MCR (Master Control Register)
``c
uint32_t mcr = CAN_MCR_INRQ
             | CAN_MCR_ABOM
             | CAN_MCR_TXFP
             | (one_shot ? CAN_MCR_NART : 0);
``