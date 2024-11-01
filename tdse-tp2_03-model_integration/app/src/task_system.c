/*
 * Copyright (c) 2023 Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * @file   : task_system.c
 * @date   : Set 26, 2023
 * @author : Juan Manuel Cruz <jcruz@fi.uba.ar> <jcruz@frba.utn.edu.ar>
 * @version	v1.0.0
 */

/********************** inclusions *******************************************/
/* Project includes. */
#include "main.h"

/* Demo includes. */
#include "logger.h"
#include "dwt.h"

/* Application & Tasks includes. */
#include "board.h"
#include "app.h"
#include "task_system_attribute.h"
#include "task_system_interface.h"
#include "task_actuator_attribute.h"
#include "task_actuator_interface.h"

/********************** macros and definitions *******************************/
#define G_TASK_SYS_CNT_INI			0ul
#define G_TASK_SYS_TICK_CNT_INI		0ul

#define DEL_SYS_XX_MIN				0ul
#define DEL_SYS_XX_MED				50ul
#define DEL_SYS_XX_MAX				500ul

#define DEL_TKT_PTR_MAX				1000ul		//Tiempo que tarda el ticket en imprimirse
#define DEL_TKT_MAX					5000ul		//Tiempo para mantener el ticket en la ticketera
#define DEL_BRC						3000ul		//Tiempo de subida de la barrera
#define DEL_BRC_UP					10000ul		//Tiempo de espera de la barrera

/********************** internal data declaration ****************************/
task_system_dta_t task_system_dta =
	{DEL_SYS_XX_MIN, ST_MOV_UDTC, EV_MOV_UDTC, false}; // VER DE MODIFICAR EL TIMER INICIAL

#define SYSTEM_DTA_QTY	(sizeof(task_system_dta)/sizeof(task_system_dta_t))

/********************** internal functions declaration ***********************/

/********************** internal data definition *****************************/
const char *p_task_system 		= "Task System (System Statechart)";
const char *p_task_system_ 		= "Non-Blocking & Update By Time Code";

/********************** external data declaration ****************************/
uint32_t g_task_system_cnt;
volatile uint32_t g_task_system_tick_cnt;

/********************** external functions definition ************************/
void task_system_init(void *parameters)
{
	task_system_dta_t 	*p_task_system_dta;
	task_system_st_t	state;
	task_system_ev_t	event;
	bool b_event;

	/* Print out: Task Initialized */
	LOGGER_LOG("  %s is running - %s\r\n", GET_NAME(task_system_init), p_task_system);
	LOGGER_LOG("  %s is a %s\r\n", GET_NAME(task_system), p_task_system_);

	g_task_system_cnt = G_TASK_SYS_CNT_INI;

	/* Print out: Task execution counter */
	LOGGER_LOG("   %s = %lu\r\n", GET_NAME(g_task_system_cnt), g_task_system_cnt);

	init_queue_event_task_system();

	/* Update Task Actuator Configuration & Data Pointer */
	p_task_system_dta = &task_system_dta;

	/* Print out: Task execution FSM */
	state = p_task_system_dta->state;
	LOGGER_LOG("   %s = %lu", GET_NAME(state), (uint32_t)state);

	event = p_task_system_dta->event;
	LOGGER_LOG("   %s = %lu", GET_NAME(event), (uint32_t)event);

	b_event = p_task_system_dta->flag;
	LOGGER_LOG("   %s = %s\r\n", GET_NAME(b_event), (b_event ? "true" : "false"));

	g_task_system_tick_cnt = G_TASK_SYS_TICK_CNT_INI;
}

void task_system_update(void *parameters)
{
	task_system_dta_t *p_task_system_dta;
	bool b_time_update_required = false;

	/* Update Task System Counter */
	g_task_system_cnt++;

	/* Protect shared resource (g_task_system_tick) */
	__asm("CPSID i");	/* disable interrupts*/
    if (G_TASK_SYS_TICK_CNT_INI < g_task_system_tick_cnt)
    {
    	g_task_system_tick_cnt--;
    	b_time_update_required = true;
    }
    __asm("CPSIE i");	/* enable interrupts*/

    while (b_time_update_required)
    {
		/* Protect shared resource (g_task_system_tick) */
		__asm("CPSID i");	/* disable interrupts*/
		if (G_TASK_SYS_TICK_CNT_INI < g_task_system_tick_cnt)
		{
			g_task_system_tick_cnt--;
			b_time_update_required = true;
		}
		else
		{
			b_time_update_required = false;
		}
		__asm("CPSIE i");	/* enable interrupts*/

    	/* Update Task System Data Pointer */
		p_task_system_dta = &task_system_dta;

		if (true == any_event_task_system())
		{
			p_task_system_dta->flag = true;
			p_task_system_dta->event = get_event_task_system();
		}

		switch (p_task_system_dta->state)
		{
			case ST_MOV_UDTC:

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_MOV_DTC))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->state = ST_MOV_DTC;
				}

				break;

			case ST_MOV_DTC:

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_BTN_PRESSED))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->tick = DEL_TKT_PTR_MAX;
					p_task_system_dta->state = ST_TKT_PRINTING;

				}

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_MOV_UDTC))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->state = ST_MOV_UDTC;

				}

				break;

			case ST_TKT_PRINTING:

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->tick > 0))
				{
					p_task_system_dta->flag = false;
					//put_event_task_actuator(EV_LED_XX_ON, ID_LED_A); // PRENDER EL LED DE LA IMPRESORA DEL TICKET
					p_task_system_dta->tick--;
				}

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->tick == 0))
				{
					p_task_system_dta->flag = false;
					//put_event_task_actuator(EV_LED_XX_OFF, ID_LED_A); // APAGAR EL LED DE LA IMPRESORA DEL TICKET
					p_task_system_dta->tick = DEL_TKT_MAX;
					p_task_system_dta->state = ST_TKT_DTC;
				}

				break;

			case ST_TKT_DTC:

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_TKT_DTC) && (p_task_system_dta->tick > 0))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->tick--;
				}

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_TKT_DTC) && (p_task_system_dta->tick == 0))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->state = ST_MOV_UDTC;
				}

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_TKT_UDTC))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->state = ST_BRC_RISING;
					p_task_system_dta->tick = DEL_BRC;
				}

				break;

			case ST_BRC_RISING:

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_BRC_RISING) && (p_task_system_dta->tick > 0))
				{
					p_task_system_dta->flag = false;
					//put_event_task_actuator(EV_LED_XX_ON, ID_LED_B); // PRENDER EL LED DE SUBIDA DE BARRERA
					p_task_system_dta->tick--;
				}

				if ((p_task_system_dta->flag == true) && ((p_task_system_dta->event == EV_BRC_UP) || (p_task_system_dta->tick == 0)))
				{
					p_task_system_dta->flag = false;
					//put_event_task_actuator(EV_LED_XX_OFF, ID_LED_B); // APAGAR EL LED DE SUBIDA DE BARRERA
					p_task_system_dta->tick = DEL_BRC_UP;
					p_task_system_dta->state = ST_BRC_UP;
				}

				break;

			case ST_BRC_UP:

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_BRC_UP) && (p_task_system_dta->tick > 0))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->tick--;
				}

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_IFR_DTC))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->state = ST_IFR_DTC;
				}

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_BRC_UP) && (p_task_system_dta->tick == 0))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->tick = DEL_BRC;
					p_task_system_dta->state = ST_BRC_FALLING;
				}

				break;

			case ST_IFR_DTC:

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_IFR_UDTC))
				{
					p_task_system_dta->flag = false;
					p_task_system_dta->tick = DEL_BRC;
					p_task_system_dta->state = ST_BRC_FALLING;
				}

				break;

			case ST_BRC_FALLING:

				if ((p_task_system_dta->flag == true) && (p_task_system_dta->event == EV_BRC_FALLING) && (p_task_system_dta->tick > 0))
				{
					p_task_system_dta->flag = false;
					//put_event_task_actuator(EV_LED_XX_ON, ID_LED_B); // PRENDER EL LED DE BAJADA DE BARRERA
					p_task_system_dta->tick--;
				}

				if ((p_task_system_dta->flag == true) && ((p_task_system_dta->event == EV_BRC_DOWN) || (p_task_system_dta->tick == 0)))
				{
					p_task_system_dta->flag = false;
					//put_event_task_actuator(EV_LED_XX_OFF, ID_LED_B); // APAGAR EL LED DE BAJADA DE BARRERA
					p_task_system_dta->state = ST_MOV_UDTC;
				}

				break;

			default:

				break;
		}
	}
}

/********************** end of file ******************************************/
