#include "usart.h"
#include "string.h"  // �����ַ�������ͷ�ļ�	
#include "stm32f10x.h"  // ����STM32F10x��ͷ�ļ�
#include "stm32f10x_usart.h"  // ����USART��ص�ͷ�ļ�
#include "stm32f10x_gpio.h"  // ����GPIO��ص�ͷ�ļ�
 

int fputc(int ch,FILE *p)  //����Ĭ�ϵģ���ʹ��printf����ʱ�Զ�����
{
	USART_SendData(USART1,(u8)ch);	
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE)==RESET);
	return ch;
}

//ʹ��scanfʱ��Ҫ�ر�USART1�жϹ���
//�ض���c�⺯��scanf��USART1
int fgetc(FILE *f)
{
	/* �ȴ�����1�������� */
	while (USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == RESET);

	return (int)USART_ReceiveData(USART1);
}

//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART1_RX_BUF[USART1_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART1_RX_STA=0;       //����״̬���


/*******************************************************************************
* �� �� ��         : USART1_Init
* ��������		   : USART1��ʼ������
* ��    ��         : bound:������
* ��    ��         : ��
*******************************************************************************/ 
void USART1_Init(u32 bound)
{
   //GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
 
	
	/*  ����GPIO��ģʽ��IO�� */
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;//TX			   //�������PA9
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF_PP;	    //�����������
	GPIO_Init(GPIOA,&GPIO_InitStructure);  /* ��ʼ����������IO */
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_10;//RX			 //��������PA10
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN_FLOATING;		  //ģ������
	GPIO_Init(GPIOA,&GPIO_InitStructure); /* ��ʼ��GPIO */
	

	//USART1 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART1, &USART_InitStructure); //��ʼ������1
	
	USART_Cmd(USART1, ENABLE);  //ʹ�ܴ���1 
	
	USART_ClearFlag(USART1, USART_FLAG_TC);
		
	// ���ý����ж�
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);

	//Usart1 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//����1�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����	
}
/*******************************************************************************
* �� �� ��         : USART1_IRQHandler
* ��������		   : USART1�жϺ���
* ��    ��         : ��
* ��    ��         : ��
*******************************************************************************/ 
// ע��: �ú�������USART1���յ�����ʱ������



// ������Щ�궨���Ѿ������ں��ʵĵط�

// extern vu16 USART1_RX_STA; // ����״̬���
// extern u8 USART1_RX_BUF[]; // ���ջ�����


void USART1_IRQHandler(void)
{
    u8 r;
    const char prefix[] = "response text:";
    uint16_t i, len;

    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        r = USART_ReceiveData(USART1);

        if ((USART1_RX_STA & 0x8000) == 0) // ����δ���
        {
            if (USART1_RX_STA & 0x4000) // ���յ�0x0D
            {
                if (r != 0x0A) // δ�յ�0x0A
                {
                    USART1_RX_STA = 0; // ���ý���״̬
                }
                else // �յ�����֡��0x0D 0x0A��β��
                {
                    USART1_RX_STA |= 0x8000; // ��ǽ������
                    len = USART1_RX_STA & 0x3FFF; // �������ݳ���

                    // ����ǰ׺
                    for (i = 0; i < strlen(prefix); i++)
                    {
                        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
                        USART_SendData(USART1, prefix[i]);
                    }

                    // ���Ϳո�ָ���
                    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
                    USART_SendData(USART1, ' ');

                    // ���ͽ��յ������ݣ��޸�����ƫ�ƣ�
                    for (i = 0; i < len; i++)
                    {
                        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
                        USART_SendData(USART1, USART1_RX_BUF[i]);
                    }

                    // ����״̬�ͻ�����
                    USART1_RX_STA = 0;
                    memset(USART1_RX_BUF, 0, USART1_REC_LEN);
                }
            }
            else // δ�յ�0x0D
            {
                if (r == 0x0D) // ��⵽�س���
                {
                    USART1_RX_STA |= 0x4000; // ������յ�0x0D
                }
                else // �洢��ͨ����
                {
                    // �޸��㣺�ȴ������ݣ��ٵ�������
                    if ((USART1_RX_STA & 0x3FFF) < USART1_REC_LEN)
                    {
                        USART1_RX_BUF[USART1_RX_STA & 0x3FFF] = r; // �ȴ�����
                        USART1_RX_STA++; // ���������
                    }
                    else // �������������
                    {
                        USART1_RX_STA = 0;
                        memset(USART1_RX_BUF, 0, USART1_REC_LEN);
                    }
                }
            }
        }
        USART_ClearITPendingBit(USART1, USART_IT_RXNE); // ����жϱ�־
    }
}
