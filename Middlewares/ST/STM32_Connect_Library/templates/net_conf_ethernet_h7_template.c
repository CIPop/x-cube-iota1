/**
  ******************************************************************************
  * @file    LwIP/LwIP_TCP_Echo_Client/Src/ethernetif.c
  * @author  MCD Application Team
  * @brief   This file implements Ethernet network interface drivers for lwIP
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32h7xx_hal.h"
#include "stm32h7xx_ll_utils.h"
#include "lwip/opt.h"
#include "lwip/timeouts.h"
#include "lwip/netif.h"
#include "lwip/tcpip.h"
#include "netif/etharp.h"
#include "net_connect.h"
#include "net_buffers.h"
#define GENERATOR_WAKAAMACLIENT_CLOUD
#ifdef NET_ETHERNET_MAC_GENERATION_FROM_MBEDTLS
#include "mbedtls/sha256.h"
#endif /* NET_ETHERNET_MAC_GENERATION_FROM_MBEDTLS */

#include "lan8742.h"
#include <string.h>

err_t ethernetif_init(struct netif *netif);
void ethernet_link_check_state(struct netif *netif);


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/


#if (osCMSIS >= 0x20000U)
#define OSSEMAPHOREWAIT         osSemaphoreAcquire
#else
#define OSSEMAPHOREWAIT         osSemaphoreWait
#endif /* osCMSIS */



/* Network interface name */
#define IFNAME0 's'
#define IFNAME1 't'

#define ETH_RX_BUFFER_SIZE                     (1524UL)

#define ETH_DMA_TRANSMIT_TIMEOUT                (20U)

/* Stack size of the interface thread */
#define INTERFACE_THREAD_STACK_SIZE 1024
#define TIME_WAITING_FOR_INPUT ( 250 ) /*( portMAX_DELAY )*/

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/



/* Semaphore to signal incoming packets */
static osSemaphoreId s_xSemaphore = NULL;
static osThreadId ethernetif_thread_handle = NULL;
static  void ethernetif_task(void *context);

#if (osCMSIS >= 0x20000U )
static const   osThreadAttr_t attr =
{
  .name = "EthIf",
  .priority = osPriorityHigh,
  . stack_size = INTERFACE_THREAD_STACK_SIZE
};
#endif /* osCMSIS */


extern  ETH_HandleTypeDef heth;
extern  ETH_TxPacketConfig TxConfig;

lan8742_Object_t LAN8742;

/* Private function prototypes -----------------------------------------------*/
u32_t sys_now(void);
void pbuf_free_custom(struct pbuf *p);

int32_t ETH_PHY_IO_Init(void);
int32_t ETH_PHY_IO_DeInit(void);
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal);
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal);
int32_t ETH_PHY_IO_GetTick(void);


lan8742_IOCtx_t  LAN8742_IOCtx = {ETH_PHY_IO_Init,
                                  ETH_PHY_IO_DeInit,
                                  ETH_PHY_IO_WriteReg,
                                  ETH_PHY_IO_ReadReg,
                                  ETH_PHY_IO_GetTick
                                 };

LWIP_MEMPOOL_DECLARE(RX_POOL, 10, sizeof(struct pbuf_custom), "Zero-copy RX PBUF pool");

/* Private functions ---------------------------------------------------------*/
/*******************************************************************************
                       LL Driver Interface ( LwIP stack --> ETH)
  *******************************************************************************/
/**
  * @brief In this function, the hardware should be initialized.
  * Called from ethernetif_init().
  *
  * @param netif the already initialized lwip network interface structure
  *        for this ethernetif
  */


extern  ETH_DMADescTypeDef  DMARxDscrTab[ETH_RX_DESC_CNT];
extern  ETH_DMADescTypeDef  DMATxDscrTab[ETH_TX_DESC_CNT];

extern uint8_t Rx_Buff[ETH_RX_DESC_CNT][ETH_MAX_PACKET_SIZE];
static void low_level_init(struct netif *netif)
{
  uint32_t idx = 0;
  uint8_t macaddress[6] = {ETH_MAC_ADDR0, ETH_MAC_ADDR1, ETH_MAC_ADDR2, ETH_MAC_ADDR3, ETH_MAC_ADDR4, ETH_MAC_ADDR5};
  /* Generate an MCU-unique MAC address */

  macaddress[0] = 0x00;
  macaddress[1] = 0x80;
  macaddress[2] = 0xE1;
  macaddress[3] = 0x15;
  macaddress[4] = 0x15;
  macaddress[5] = 0x38;
#if 0
#ifdef NET_ETHERNET_MAC_GENERATION_FROM_MBEDTLS
#define HASH_OUTPUT_LENGTH  32

  unsigned char output[HASH_OUTPUT_LENGTH];
  uint32_t UID[3];
  /* Generate a hash digest from the unique CPU ID */
  UID[0] = LL_GetUID_Word0();
  UID[1] = LL_GetUID_Word1();
  UID[2] = LL_GetUID_Word2();

  /* Hash with mbedTLS */
  int32_t hash_success = 0;
  mbedtls_sha256_context sha_context;
  mbedtls_sha256_init(&sha_context);

  if (0 == mbedtls_sha256_starts_ret(&sha_context, 0))
  {
    if (0 == mbedtls_sha256_update_ret(&sha_context, (const unsigned char *) UID, sizeof(UID)))
    {
      if (0 == mbedtls_sha256_finish_ret(&sha_context, output))
      {
        hash_success = 1;
      }
    }
  }
  mbedtls_sha256_free(&sha_context);

  while (hash_success == 0)
  {
    ;
  }

  /* Copy the 3 first bytes of the digest to the second half of the MAC address. */
  memcpy(&macaddress[3], output, 3);

#endif /* NET_ETHERNET_MAC_GENERATION_FROM_MBEDTLS */
#endif /* 0 */
  heth.Instance = ETH;
  heth.Init.MACAddr = macaddress;
  heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
  heth.Init.RxDesc = DMARxDscrTab;
  heth.Init.TxDesc = DMATxDscrTab;
  heth.Init.RxBuffLen = ETH_RX_BUFFER_SIZE;

  /* configure ethernet peripheral (GPIOs, clocks, MAC, DMA) */
  HAL_ETH_Init(&heth);

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  /* set MAC hardware address */
  for (uint32_t i = 0; i < sizeof(macaddress) / sizeof(uint8_t); i++)
  {
    netif->hwaddr[i] =  macaddress[i];
  }

  /* maximum transfer unit */
  netif->mtu = ETH_MAX_PAYLOAD;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  netif->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

  for (idx = 0; idx < ETH_RX_DESC_CNT; idx ++)
  {
    HAL_ETH_DescAssignMemory(&heth, idx, Rx_Buff[idx], NULL);
  }

  /* Initialize the RX POOL */
  LWIP_MEMPOOL_INIT(RX_POOL);


  /* Set PHY IO functions */
  LAN8742_RegisterBusIO(&LAN8742, &LAN8742_IOCtx);

  /* Initialize the LAN8742 ETH PHY */
  LAN8742_Init(&LAN8742);

  ethernet_link_check_state(netif);
}

/**
  * @brief This function should do the actual transmission of the packet. The packet is
  * contained in the pbuf that is passed to the function. This pbuf
  * might be chained.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
  * @return ERR_OK if the packet could be sent
  *         an err_t value if the packet couldn't be sent
  *
  * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
  *       strange results. You might consider waiting for space in the DMA queue
  *       to become available since the stack doesn't retry to send a packet
  *       dropped because of memory failure (except for the TCP timers).
  */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
  uint32_t i = 0;
  uint32_t  framelen = 0;
  struct pbuf *q;
  err_t errval = ERR_OK;
  ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT];

  memset(Txbuffer, 0, ETH_TX_DESC_CNT * sizeof(ETH_BufferTypeDef));

  for (q = p; q != NULL; q = q->next)
  {
    if (i >= ETH_TX_DESC_CNT)
    {
      return ERR_IF;
    }

    Txbuffer[i].buffer = q->payload;
    Txbuffer[i].len = q->len;
    framelen += q->len;

    if (i > 0)
    {
      Txbuffer[i - 1].next = &Txbuffer[i];
    }

    if (q->next == NULL)
    {
      Txbuffer[i].next = NULL;
    }

    i++;
  }

  TxConfig.Length = framelen;
  TxConfig.TxBuffer = Txbuffer;

  HAL_ETH_Transmit(&heth, &TxConfig, ETH_DMA_TRANSMIT_TIMEOUT);

  return errval;
}

/**
  * @brief Should allocate a pbuf and transfer the bytes of the incoming
  * packet from the interface into the pbuf.
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return a pbuf filled with the received packet (including MAC header)
  *         NULL on memory error
  */
static struct pbuf *low_level_input(struct netif *netif)
{
  struct pbuf *p = NULL;
  ETH_BufferTypeDef RxBuff;
  uint32_t framelength = 0;
  struct pbuf_custom *custom_pbuf;

  if (HAL_ETH_IsRxDataAvailable(&heth))
  {
    HAL_ETH_GetRxDataBuffer(&heth, &RxBuff);
    HAL_ETH_GetRxDataLength(&heth, &framelength);

    /* Build Rx descriptor to be ready for next data reception */
    HAL_ETH_BuildRxDescriptors(&heth);

    /* Invalidate data cache for ETH Rx Buffers */
    SCB_InvalidateDCache_by_Addr((uint32_t *)RxBuff.buffer, framelength);

    custom_pbuf  = (struct pbuf_custom *)LWIP_MEMPOOL_ALLOC(RX_POOL);
    custom_pbuf->custom_free_function = pbuf_free_custom;

    p = pbuf_alloced_custom(PBUF_RAW, framelength, PBUF_REF, custom_pbuf, RxBuff.buffer, ETH_RX_BUFFER_SIZE);

    return p;
  }
  else
  {
    return NULL;
  }
}

/**
  * @brief Should be called at the beginning of the program to set up the
  * network interface. It calls the function low_level_init() to do the
  * actual setup of the hardware.
  *
  * This function should be passed as a parameter to netif_add().
  *
  * @param netif the lwip network interface structure for this ethernetif
  * @return ERR_OK if the loopif is initialized
  *         ERR_MEM if private data couldn't be allocated
  *         any other err_t on error
  */
err_t ethernetif_init(struct netif *netif)
{
  LWIP_ASSERT("netif != NULL", (netif != NULL));

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  netif->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
  netif->output = etharp_output;
  netif->linkoutput = low_level_output;

  /* initialize the hardware */
  low_level_init(netif);


  /* create a  semaphore used for informing ethernetif of frame reception */
  /* ethernet link list is 4 buffers, set semaphore size to 4 to not miss */
  /* interrupt (1 is not working )                                        */
#if (osCMSIS >= 0x20000U)
  s_xSemaphore = osSemaphoreNew(4, 4, NULL);
#else
  osSemaphoreDef(SEM);
  s_xSemaphore = osSemaphoreCreate(osSemaphore(SEM), 4);
#endif /* osCMSIS */

  /* create the task that handles the received data */
#if (osCMSIS < 0x20000U )

  osThreadDef(EthIf, (os_pthread) ethernetif_task, osPriorityHigh, 0, INTERFACE_THREAD_STACK_SIZE);
  ethernetif_thread_handle = osThreadCreate(osThread(EthIf), netif);
#else
  ethernetif_thread_handle = osThreadNew((osThreadFunc_t)ethernetif_task, netif, &attr);
#endif /* osCMSIS */
  HAL_Delay(1000);
  return ERR_OK;
}


err_t ethernetif_deinit(struct netif *netif)
{
  /* join will be welcome first ? */
  (void) osThreadTerminate(ethernetif_thread_handle);
  return ERR_OK;
}


/**
  * @brief  Custom Rx pbuf free callback
  * @param  pbuf: pbuf to be freed
  * @retval None
  */
void pbuf_free_custom(struct pbuf *p)
{
  struct pbuf_custom *custom_pbuf = (struct pbuf_custom *)p;
  /* Invalidate data cache: lwIP and/or application may have written into buffer */
  SCB_InvalidateDCache_by_Addr((uint32_t *)p->payload, p->tot_len);
  LWIP_MEMPOOL_FREE(RX_POOL, custom_pbuf);
}

/**
  * @brief  Returns the current time in milliseconds
  *         when LWIP_TIMERS == 1 and NO_SYS == 1
  * @param  None
  * @retval Current Time value
  */
u32_t sys_now(void)
{
  return HAL_GetTick();
}
/*******************************************************************************
                       Ethernet MSP Routines
  ********************************************************************************/

/**
  * @brief  Ethernet Rx Transfer completed callback
  * @param  heth: ETH handle
  * @retval None
  */
void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
  osSemaphoreRelease(s_xSemaphore);
}
/*******************************************************************************
                       PHI IO Functions
  *******************************************************************************/
/**
  * @brief  Initializes the MDIO interface GPIO and clocks.
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_Init(void)
{
  /* We assume that MDIO GPIO configuration is already done
     in the ETH_MspInit() else it should be done here
  */

  /* Configure the MDIO Clock */
  HAL_ETH_SetMDIOClockRange(&heth);

  return 0;
}

/**
  * @brief  De-Initializes the MDIO interface .
  * @param  None
  * @retval 0 if OK, -1 if ERROR
  */
int32_t ETH_PHY_IO_DeInit(void)
{
  return 0;
}

/**
  * @brief  Read a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  pRegVal: pointer to hold the register value
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_ReadReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t *pRegVal)
{
  if (HAL_ETH_ReadPHYRegister(&heth, DevAddr, RegAddr, pRegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Write a value to a PHY register through the MDIO interface.
  * @param  DevAddr: PHY port address
  * @param  RegAddr: PHY register address
  * @param  RegVal: Value to be written
  * @retval 0 if OK -1 if Error
  */
int32_t ETH_PHY_IO_WriteReg(uint32_t DevAddr, uint32_t RegAddr, uint32_t RegVal)
{
  if (HAL_ETH_WritePHYRegister(&heth, DevAddr, RegAddr, RegVal) != HAL_OK)
  {
    return -1;
  }

  return 0;
}

/**
  * @brief  Get the time in millisecons used for internal PHY driver process.
  * @retval Time value
  */
int32_t ETH_PHY_IO_GetTick(void)
{
  return HAL_GetTick();
}

/**
  * @brief
  * @retval None
  */
void ethernet_link_check_state(struct netif *netif)
{
  ETH_MACConfigTypeDef MACConf;
  uint32_t PHYLinkState;
  uint32_t linkchanged = 0;
  uint32_t speed = 0;
  uint32_t duplex = 0;

  PHYLinkState = LAN8742_GetLinkState(&LAN8742);

  if (netif_is_link_up(netif) && (PHYLinkState <= LAN8742_STATUS_LINK_DOWN))
  {
    HAL_ETH_Stop_IT(&heth);
    netif_set_down(netif);
    netif_set_link_down(netif);
  }
  else if (!netif_is_link_up(netif) && (PHYLinkState > LAN8742_STATUS_LINK_DOWN))
  {
    switch (PHYLinkState)
    {
      case LAN8742_STATUS_100MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_100MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_100M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_10MBITS_FULLDUPLEX:
        duplex = ETH_FULLDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        linkchanged = 1;
        break;
      case LAN8742_STATUS_10MBITS_HALFDUPLEX:
        duplex = ETH_HALFDUPLEX_MODE;
        speed = ETH_SPEED_10M;
        linkchanged = 1;
        break;
      default:
        break;
    }

    if (linkchanged)
    {
      /* Get MAC Config MAC */
      HAL_ETH_GetMACConfig(&heth, &MACConf);
      MACConf.DuplexMode = duplex;
      MACConf.Speed = speed;
      HAL_ETH_SetMACConfig(&heth, &MACConf);
      HAL_ETH_Start_IT(&heth);
      netif_set_up(netif);
      netif_set_link_up(netif);
    }
  }
}

uint8_t ethernetif_low_get_link_status(void)
{
  uint8_t       ret;
  uint32_t PHYLinkState;
  PHYLinkState = LAN8742_GetLinkState(&LAN8742);
  if (PHYLinkState > LAN8742_STATUS_LINK_DOWN)
  {
    ret = 1;
  }
  else
  {
    ret = 0;
  }
  return ret;
}


static  void ethernetif_task(void *context)
{
  struct netif *netif = context;
  int32_t semaphore_retval;
  static uint8_t link_status = 0;

  if (link_status)
  {
    netif_set_link_up(netif);
  }
  else netif_set_link_down(netif);
  for (;;)
  {
    semaphore_retval = OSSEMAPHOREWAIT(s_xSemaphore, TIME_WAITING_FOR_INPUT);

    if (ethernetif_low_get_link_status() != link_status)
    {
      link_status = 1U - link_status;
      if (link_status)
      {
        netif_set_link_up(netif);
      }
      else netif_set_link_down(netif);
    }
    if (semaphore_retval == (int32_t) osOK)
    {
      struct pbuf *p;
      /* move received packet into a new pbuf */
      p = low_level_input(netif);
      /* no packet could be read, silently ignore this */
      if (p)
      {
        err_t err;
        /* entry point to the LwIP stack */
        err = netif->input(p, netif);
        if (err != ERR_OK)
        {
          LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
          pbuf_free(p);
        }
      }
    }
  }
}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
