#include "../../core/arm/armstate.h"
#include "../../core/arm/free_bootloader_image.h"
#include "../../core/asic.h"
#include "../../core/coproc.h"
#include "../../core/schedule.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static unsigned int failures;
asic_state_t asic;

uint64_t sched_total_time(enum clock_id clock) {
    (void)clock;
    return 0;
}

void gui_console_printf(const char *format, ...) {
    (void)format;
}

#define CHECK(condition, message) do { \
    if (!(condition)) { \
        fprintf(stderr, "FAIL: %s\n", message); \
        ++failures; \
    } \
} while (0)

static void init_arm(arm_t *arm) {
    memset(arm, 0, sizeof(*arm));
    if (!arm_mem_init(&arm->mem)) {
        fprintf(stderr, "FAIL: couldn't initialize ARM memory\n");
        ++failures;
    }
}

static void destroy_arm(arm_t *arm) {
    arm_mem_destroy(&arm->mem);
}

typedef bool (*arm_condition_t)(const arm_t *arm);

static bool wait_for_arm_condition(arm_t *arm, arm_condition_t condition) {
    for (unsigned int attempt = 0; attempt != 1000000; ++attempt) {
        sync_enter(&arm->sync);
        bool ready = condition(arm);
        sync_leave(&arm->sync);
        if (ready) {
            return true;
        }
        arm_run_until(arm, arm_get_time(arm) + 256);
    }
    return false;
}

static void prepare_instruction(arm_t *arm, uint16_t opcode) {
    memset(&arm->cpu, 0, sizeof(arm->cpu));
    arm->cycles = 0;
    arm->mem.nvm[0] = UINT32_C(0xBF00) << 16 | opcode;
    arm->cpu.pc = 2;
}

static void prepare_instruction32(arm_t *arm, uint16_t first, uint16_t second) {
    memset(&arm->cpu, 0, sizeof(arm->cpu));
    arm->cycles = 0;
    arm->mem.nvm[0] = (uint32_t)second << 16 | first;
    arm->cpu.pc = 2;
}

static void test_adc_flags(void) {
    arm_t arm;
    init_arm(&arm);

    prepare_instruction(&arm, UINT16_C(0x4148)); /* ADCS r0, r1 */
    arm.cpu.r0 = UINT32_C(0xFFFFFFFF);
    arm.cpu.r1 = 0;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.r0 == UINT32_C(0xFFFFFFFF), "ADC preserves -1 + 0");
    CHECK(arm.cpu.n && !arm.cpu.z && !arm.cpu.c && !arm.cpu.v,
          "ADC flags for -1 + 0 are N=1 Z=0 C=0 V=0");

    prepare_instruction(&arm, UINT16_C(0x4148));
    arm.cpu.r0 = UINT32_C(0x7FFFFFFF);
    arm.cpu.r1 = 0;
    arm.cpu.c = true;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.r0 == UINT32_C(0x80000000), "ADC applies carry-in");
    CHECK(arm.cpu.n && !arm.cpu.z && !arm.cpu.c && arm.cpu.v,
          "ADC flags signed overflow from INT_MAX + carry");

    prepare_instruction(&arm, UINT16_C(0x4148));
    arm.cpu.r0 = UINT32_C(0xFFFFFFFF);
    arm.cpu.r1 = 0;
    arm.cpu.c = true;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.r0 == 0, "ADC wraps UINT_MAX + carry");
    CHECK(!arm.cpu.n && arm.cpu.z && arm.cpu.c && !arm.cpu.v,
          "ADC flags unsigned carry without signed overflow");

    destroy_arm(&arm);
}

static void test_sbc_flags(void) {
    arm_t arm;
    init_arm(&arm);

    prepare_instruction(&arm, UINT16_C(0x4188)); /* SBCS r0, r1 */
    arm.cpu.r0 = UINT32_C(0xFFFFFFFF);
    arm.cpu.r1 = 0;
    arm.cpu.c = true;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.r0 == UINT32_C(0xFFFFFFFF), "SBC preserves -1 - 0");
    CHECK(arm.cpu.n && !arm.cpu.z && arm.cpu.c && !arm.cpu.v,
          "SBC flags for -1 - 0 are N=1 Z=0 C=1 V=0");

    prepare_instruction(&arm, UINT16_C(0x4188));
    arm.cpu.r0 = UINT32_C(0x80000000);
    arm.cpu.r1 = 1;
    arm.cpu.c = true;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.r0 == UINT32_C(0x7FFFFFFF), "SBC computes INT_MIN - 1");
    CHECK(!arm.cpu.n && !arm.cpu.z && arm.cpu.c && arm.cpu.v,
          "SBC flags signed overflow from INT_MIN - 1");

    prepare_instruction(&arm, UINT16_C(0x4188));
    arm.cpu.r0 = 0;
    arm.cpu.r1 = 0;
    arm.cpu.c = false;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.r0 == UINT32_C(0xFFFFFFFF), "SBC applies borrow-in");
    CHECK(arm.cpu.n && !arm.cpu.z && !arm.cpu.c && !arm.cpu.v,
          "SBC flags unsigned borrow without signed overflow");

    destroy_arm(&arm);
}

static void test_random_adc_sbc_flags(void) {
    arm_t arm;
    uint32_t random = UINT32_C(0xC0DEC0DE);
    init_arm(&arm);

    for (unsigned int iteration = 0; iteration != 4096; ++iteration) {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        uint32_t x = random;
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        uint32_t y = random;
        bool carry = iteration & 1;

        prepare_instruction(&arm, UINT16_C(0x4148)); /* ADCS r0, r1 */
        arm.cpu.r0 = x;
        arm.cpu.r1 = y;
        arm.cpu.c = carry;
        arm_cpu_execute(&arm);
        uint64_t unsigned_sum = (uint64_t)x + y + carry;
        int64_t signed_sum = (int64_t)(int32_t)x + (int32_t)y + carry;
        if (arm.cpu.r0 != (uint32_t)unsigned_sum ||
            arm.cpu.n != ((uint32_t)unsigned_sum >> 31) ||
            arm.cpu.z != ((uint32_t)unsigned_sum == 0) ||
            arm.cpu.c != (unsigned_sum >> 32) ||
            arm.cpu.v != (signed_sum < INT32_MIN || signed_sum > INT32_MAX)) {
            CHECK(false, "randomized ADC result or flags mismatch");
            break;
        }

        prepare_instruction(&arm, UINT16_C(0x4188)); /* SBCS r0, r1 */
        arm.cpu.r0 = x;
        arm.cpu.r1 = y;
        arm.cpu.c = carry;
        arm_cpu_execute(&arm);
        uint32_t borrow = !carry;
        uint32_t difference = x - y - borrow;
        int64_t signed_difference = (int64_t)(int32_t)x - (int32_t)y - borrow;
        if (arm.cpu.r0 != difference ||
            arm.cpu.n != (difference >> 31) ||
            arm.cpu.z != (difference == 0) ||
            arm.cpu.c != ((uint64_t)x >= (uint64_t)y + borrow) ||
            arm.cpu.v != (signed_difference < INT32_MIN ||
                          signed_difference > INT32_MAX)) {
            CHECK(false, "randomized SBC result or flags mismatch");
            break;
        }
    }

    destroy_arm(&arm);
}

static void test_relative_branches(void) {
    arm_t arm;
    init_arm(&arm);

    prepare_instruction(&arm, UINT16_C(0xD0FE)); /* BEQ -4 */
    arm.cpu.z = true;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.pc == UINT32_C(0x2),
          "conditional branch sign-extends its backward offset");

    prepare_instruction(&arm, UINT16_C(0xE7FE)); /* B -4 */
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.pc == UINT32_C(0x2),
          "unconditional branch sign-extends its backward offset");

    memset(&arm.cpu, 0, sizeof(arm.cpu));
    arm.mem.nvm[0] = UINT32_C(0xFFFEF7FF); /* BL from address 0 back to address 0. */
    arm.cpu.pc = 2;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.pc == UINT32_C(0x2),
          "branch with link sign-extends its backward offset");
    CHECK(arm.cpu.lr == UINT32_C(0x5),
          "branch with link records the Thumb return address");

    destroy_arm(&arm);
}

static void test_instruction_cycle_counts(void) {
    arm_t arm;
    init_arm(&arm);

    prepare_instruction(&arm, UINT16_C(0x4348)); /* MULS r0, r1, r0 */
    arm.cpu.r0 = 3;
    arm.cpu.r1 = 7;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 1, "SAMD21 fast multiplier takes one cycle");

    prepare_instruction(&arm, UINT16_C(0x6808)); /* LDR r0, [r1] */
    arm.cpu.r1 = HMCRAMC0_ADDR;
    arm.mem.ram[0] = UINT32_C(0x12345678);
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "AHB load takes two cycles");

    prepare_instruction(&arm, UINT16_C(0x6008)); /* STR r0, [r1] */
    arm.cpu.r0 = UINT32_C(0xA5A5A5A5);
    arm.cpu.r1 = HMCRAMC0_ADDR;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "AHB store takes two cycles");

    prepare_instruction(&arm, UINT16_C(0x6808)); /* LDR r0, [r1] */
    arm.cpu.r1 = (uint32_t)PORT_IOBUS;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 1, "IOBUS load takes one cycle");

    prepare_instruction(&arm, UINT16_C(0x6008)); /* STR r0, [r1] */
    arm.cpu.r1 = (uint32_t)PORT_IOBUS;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 1, "IOBUS store takes one cycle");

    prepare_instruction(&arm, UINT16_C(0xB503)); /* PUSH {r0, r1, lr} */
    arm.cpu.sp = HMCRAMC0_ADDR + 0x20;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 4, "PUSH takes one plus register-count cycles");

    prepare_instruction(&arm, UINT16_C(0xBC03)); /* POP {r0, r1} */
    arm.cpu.sp = HMCRAMC0_ADDR;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 3, "POP takes one plus register-count cycles");

    prepare_instruction(&arm, UINT16_C(0xBD03)); /* POP {r0, r1, pc} */
    arm.cpu.sp = HMCRAMC0_ADDR;
    arm.mem.ram[2] = UINT32_C(0x101);
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 6, "POP including PC takes three plus register-count cycles");

    prepare_instruction(&arm, UINT16_C(0xC11C)); /* STM r1!, {r2-r4} */
    arm.cpu.r1 = HMCRAMC0_ADDR;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 4, "STM takes one plus register-count cycles");

    prepare_instruction(&arm, UINT16_C(0xC90C)); /* LDM r1!, {r2, r3} */
    arm.cpu.r1 = HMCRAMC0_ADDR;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 3, "LDM takes one plus register-count cycles");

    prepare_instruction(&arm, UINT16_C(0xD000)); /* BEQ +0 */
    arm.cpu.z = false;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 1, "untaken conditional branch takes one cycle");

    prepare_instruction(&arm, UINT16_C(0xD000)); /* BEQ +0 */
    arm.cpu.z = true;
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "taken conditional branch takes two cycles");

    prepare_instruction(&arm, UINT16_C(0xE000)); /* B +0 */
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "unconditional branch takes two cycles");

    prepare_instruction(&arm, UINT16_C(0x4687)); /* MOV pc, r0 */
    arm.cpu.r0 = UINT32_C(0x101);
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "MOV to PC takes two cycles");

    prepare_instruction(&arm, UINT16_C(0x4487)); /* ADD pc, r0 */
    arm.cpu.r0 = UINT32_C(0xFC);
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "ADD to PC takes two cycles");

    prepare_instruction(&arm, UINT16_C(0x4700)); /* BX r0 */
    arm.cpu.r0 = UINT32_C(0x101);
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "BX takes two cycles");

    prepare_instruction(&arm, UINT16_C(0x4780)); /* BLX r0 */
    arm.cpu.r0 = UINT32_C(0x101);
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "BLX takes two cycles");

    prepare_instruction32(&arm, UINT16_C(0xF7FF), UINT16_C(0xFFFE)); /* BL -4 */
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 3, "BL takes three cycles");

    prepare_instruction32(&arm, UINT16_C(0xF3EF), UINT16_C(0x8014)); /* MRS r0, CONTROL */
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 3, "MRS takes three cycles");

    prepare_instruction32(&arm, UINT16_C(0xF380), UINT16_C(0x8814)); /* MSR CONTROL, r0 */
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 3, "MSR takes three cycles");

    prepare_instruction32(&arm, UINT16_C(0xF3BF), UINT16_C(0x8F5F)); /* DMB SY */
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 3, "DMB takes three cycles");

    prepare_instruction32(&arm, UINT16_C(0xF3BF), UINT16_C(0x8F4F)); /* DSB SY */
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 3, "DSB takes three cycles");

    prepare_instruction32(&arm, UINT16_C(0xF3BF), UINT16_C(0x8F6F)); /* ISB SY */
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 3, "ISB takes three cycles");

    prepare_instruction(&arm, UINT16_C(0xBF20)); /* WFE */
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "WFE takes two cycles before waiting");

    prepare_instruction(&arm, UINT16_C(0xBF30)); /* WFI */
    arm_cpu_execute(&arm);
    CHECK(arm.cycles == 2, "WFI takes two cycles before waiting");

    prepare_instruction32(&arm, UINT16_C(0xF7FF), UINT16_C(0xFFFE)); /* BL -4 */
    arm.cpu.systick.ctrl = SysTick_CTRL_ENABLE_Msk;
    arm.cpu.systick.load = 10;
    arm.cpu.systick.val = 10;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.systick.val == 7,
          "SysTick advances by every cycle of a multi-cycle instruction");

    prepare_instruction32(&arm, UINT16_C(0xF7FF), UINT16_C(0xFFFE)); /* BL -4 */
    arm.cpu.systick.ctrl = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_TICKINT_Msk;
    arm.cpu.systick.load = 2;
    arm.cpu.systick.val = 1;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.systick.val == 1,
          "multi-cycle SysTick accounting reloads and continues counting");
    CHECK((arm.cpu.systick.ctrl & SysTick_CTRL_COUNTFLAG_Msk) &&
              (arm.cpu.scb.icsr & SCB_ICSR_PENDSTSET_Msk),
          "multi-cycle SysTick accounting records and pends a wrap");

    destroy_arm(&arm);
}

static void test_high_exception_return(void) {
    arm_t arm;
    const arm_exception_number_t exception =
        (arm_exception_number_t)(ARM_Exception_External + 24);
    init_arm(&arm);
    prepare_instruction(&arm, UINT16_C(0x4770)); /* BX lr */

    arm.cpu.lr = UINT32_C(0xFFFFFFF9); /* Return to Thread mode using MSP. */
    arm.cpu.sp = HMCRAMC0_ADDR;
    arm.cpu.active = UINT64_C(1) << exception;
    arm.cpu.scb.icsr = exception;
    arm.mem.ram[6] = UINT32_C(0x100);
    arm.mem.ram[7] = UINT32_C(1) << 24;

    arm_cpu_execute(&arm);
    CHECK(arm.cpu.active == 0, "exception return clears active exception above 31");
    CHECK((arm.cpu.scb.icsr & SCB_ICSR_VECTACTIVE_Msk) == 0,
          "exception return restores Thread mode");
    CHECK(arm.cpu.pc == UINT32_C(0x102), "exception return restores the stacked PC");

    destroy_arm(&arm);
}

static void test_core_register_reset_values(void) {
    arm_t arm;
    init_arm(&arm);

    CHECK(arm_mem_load_word(&arm, SCB_BASE) == UINT32_C(0x410CC601),
          "CPUID identifies a Cortex-M0+ r0p1");

    arm.mem.nvm[0] = HMCRAMC0_ADDR + HMCRAMC0_SIZE;
    arm.mem.nvm[1] = UINT32_C(0x101);
    arm_cpu_reset(&arm);
    CHECK(arm.cpu.systick.ctrl == SysTick_CTRL_CLKSOURCE_Msk,
          "SysTick CTRL resets with CLKSOURCE set");

    destroy_arm(&arm);
}

static void prepare_exception_vectors(arm_t *arm) {
    arm->cpu.sp = HMCRAMC0_ADDR + HMCRAMC0_SIZE;
    arm->cpu.pc = UINT32_C(0x102);
    arm->mem.nvm[UINT32_C(0x100) >> 2] = UINT16_C(0xBF00); /* NOP */
    arm->mem.nvm[ARM_Exception_SVCall] = UINT32_C(0x201);
    arm->mem.nvm[ARM_Exception_PendSV] = UINT32_C(0x221);
    arm->mem.nvm[ARM_Exception_SysTick] = UINT32_C(0x241);
    arm->mem.nvm[ARM_Exception_External] = UINT32_C(0x261);
    arm->mem.nvm[ARM_Exception_External + 1] = UINT32_C(0x281);
}

static void test_exception_priorities(void) {
    arm_t arm;
    init_arm(&arm);
    prepare_exception_vectors(&arm);

    arm.cpu.scb.icsr = SCB_ICSR_PENDSVSET_Msk | SCB_ICSR_PENDSTSET_Msk;
    arm.cpu.scb.shp[1] = UINT32_C(3) << 22 | UINT32_C(1) << 30;
    arm.cpu.nvic.ier = 1;
    arm.cpu.nvic.ipr = 1;
    arm.cpu.nvic.ip[0] = 0;
    arm_cpu_execute(&arm);
    CHECK((arm.cpu.scb.icsr & SCB_ICSR_VECTACTIVE_Msk) == ARM_Exception_External,
          "highest-priority pending external interrupt is serviced first");

    memset(&arm.cpu, 0, sizeof(arm.cpu));
    prepare_exception_vectors(&arm);
    arm.cpu.scb.icsr = SCB_ICSR_PENDSVSET_Msk | SCB_ICSR_PENDSTSET_Msk;
    arm.cpu.scb.shp[1] = UINT32_C(3) << 22 | UINT32_C(1) << 30;
    arm_cpu_execute(&arm);
    CHECK((arm.cpu.scb.icsr & SCB_ICSR_VECTACTIVE_Msk) == ARM_Exception_SysTick,
          "SysTick is serviced before a lower-priority PendSV");

    memset(&arm.cpu, 0, sizeof(arm.cpu));
    prepare_exception_vectors(&arm);
    arm.cpu.active = UINT64_C(1) << ARM_Exception_SysTick;
    arm.cpu.scb.icsr = ARM_Exception_SysTick | SCB_ICSR_PENDSVSET_Msk;
    arm.cpu.scb.shp[1] = UINT32_C(3) << 22 | UINT32_C(1) << 30;
    arm_cpu_execute(&arm);
    CHECK((arm.cpu.scb.icsr & SCB_ICSR_VECTACTIVE_Msk) == ARM_Exception_SysTick,
          "lower-priority pending exception does not preempt an active handler");
    CHECK(arm.cpu.pc == UINT32_C(0x104),
          "active handler continues when pending work cannot preempt it");

    memset(&arm.cpu, 0, sizeof(arm.cpu));
    prepare_exception_vectors(&arm);
    arm.cpu.active = UINT64_C(1) << ARM_Exception_SysTick;
    arm.cpu.scb.icsr = ARM_Exception_SysTick;
    arm.cpu.scb.shp[1] = UINT32_C(1) << 30;
    arm.cpu.nvic.ier = UINT32_C(1) << 1;
    arm.cpu.nvic.ipr = UINT32_C(1) << 1;
    arm.cpu.nvic.ip[0] = 0;
    arm_cpu_execute(&arm);
    CHECK((arm.cpu.scb.icsr & SCB_ICSR_VECTACTIVE_Msk) ==
              ARM_Exception_External + 1,
          "higher-priority external interrupt preempts an active handler");

    destroy_arm(&arm);
}

static void test_svc_pending_register(void) {
    arm_t arm;
    init_arm(&arm);

    arm_mem_store_word(&arm, SCB_SHCSR_SVCALLPENDED_Msk, SCB_BASE + 0x24);
    CHECK(arm.cpu.svc_pending, "SHCSR records a pending SVCall");
    CHECK(!(arm.cpu.scb.icsr & SCB_ICSR_PENDSVSET_Msk),
          "SHCSR SVCall state is independent from PendSV");
    CHECK(arm_mem_load_word(&arm, SCB_BASE + 0x24) == SCB_SHCSR_SVCALLPENDED_Msk,
          "SHCSR reports the pending SVCall");
    arm_mem_store_word(&arm, 0, SCB_BASE + 0x24);
    CHECK(!arm.cpu.svc_pending, "SHCSR clears a pending SVCall");

    destroy_arm(&arm);
}

static void test_svc_instruction_pending(void) {
    arm_t arm;
    init_arm(&arm);
    prepare_instruction(&arm, UINT16_C(0xDF00)); /* SVC #0 */
    arm.cpu.sp = HMCRAMC0_ADDR + HMCRAMC0_SIZE;
    arm.mem.nvm[ARM_Exception_SVCall] = UINT32_C(0x201);

    arm.cpu.pm = true;
    arm_cpu_execute(&arm);
    CHECK(arm.cpu.svc_pending, "masked SVC instruction remains pending");
    CHECK((arm.cpu.scb.icsr & SCB_ICSR_VECTACTIVE_Msk) == ARM_Thread_Mode,
          "masked SVC does not enter its handler");

    arm.cpu.pm = false;
    arm_cpu_execute(&arm);
    CHECK(!arm.cpu.svc_pending, "SVC pending state clears on exception entry");
    CHECK((arm.cpu.scb.icsr & SCB_ICSR_VECTACTIVE_Msk) == ARM_Exception_SVCall,
          "unmasked pending SVC enters its handler");
    CHECK(arm.cpu.pc == UINT32_C(0x202), "SVC loads its exception vector");

    destroy_arm(&arm);
}

static void test_peripheral_reset(void) {
    arm_t arm;
    init_arm(&arm);

    arm.mem.pm.AHBMASK.reg = UINT32_MAX;
    arm.mem.gclk.GENCTRL[0].reg = UINT32_MAX;
    arm.mem.nvmctrl.CTRLB.reg = UINT32_MAX;
    arm.mem.sercom[0].USART.CTRLA.reg = UINT32_MAX;
    arm.mem.nvm[0] = UINT32_C(0x01234567);
    arm.mem.ram[0] = UINT32_C(0x89ABCDEF);
    arm.mem.aux[0] = UINT32_C(0x55AA55AA);
    arm_mem_reset(&arm.mem, PM_RCAUSE_EXT);

    CHECK(arm.mem.pm.AHBMASK.reg == PM_AHBMASK_RESETVALUE,
          "reset restores the PM AHB mask");
    CHECK(arm.mem.pm.APBAMASK.reg == PM_APBAMASK_RESETVALUE,
          "reset restores the PM APBA mask");
    CHECK(arm.mem.pm.APBBMASK.reg == PM_APBBMASK_RESETVALUE,
          "reset restores the PM APBB mask");
    CHECK(arm.mem.pm.APBCMASK.reg == PM_APBCMASK_RESETVALUE,
          "reset restores the PM APBC mask");
    CHECK(arm.mem.gclk.GENCTRL[0].reg == 0, "reset clears GCLK registers");
    CHECK(arm.mem.nvmctrl.CTRLB.reg == 0, "reset clears NVMCTRL registers");
    CHECK(arm.mem.nvmctrl.INTFLAG.bit.READY, "reset sets NVMCTRL READY");
    CHECK(arm.mem.nvmctrl.LOCK.reg == NVMCTRL_LOCK_LOCK_Msk,
          "reset restores the NVM region lock mask");
    CHECK(arm.mem.sercom[0].USART.CTRLA.reg == 0, "reset clears SERCOM registers");
    CHECK(arm.mem.pm.RCAUSE.reg == PM_RCAUSE_EXT, "reset records its cause");
    CHECK(arm.mem.nvm[0] == UINT32_C(0x01234567), "reset preserves flash contents");
    CHECK(arm.mem.ram[0] == UINT32_C(0x89ABCDEF), "reset preserves SRAM contents");
    CHECK(arm.mem.aux[0] == UINT32_C(0x55AA55AA),
          "reset preserves auxiliary flash contents");

    destroy_arm(&arm);
}

static void test_sercom_status_polling(void) {
    arm_t arm;
    const uint32_t status_address = HPB2_ADDR +
        ((ID_SERCOM0 - ID_PAC2) << 10) + SERCOM_USART_INTFLAG_OFFSET;
    init_arm(&arm);

    bool sync_initialized = sync_init(&arm.sync);
    CHECK(sync_initialized, "ARM synchronization initializes");
    if (sync_initialized) {
        arm.mem.sercom[0].USART.CTRLA.bit.MODE =
            SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val;
        for (unsigned int read = 0; read != 8; ++read) {
            (void)arm_mem_load_word(&arm, status_address);
        }
        CHECK(!arm.sync.slp,
              "bounded SERCOM status polling does not suspend ARM execution");
        sync_wake(&arm.sync);
        sync_destroy(&arm.sync);
    }

    destroy_arm(&arm);
}

static void test_usart_transmitter_empty_status(void) {
    arm_t arm;
    const uint32_t status_address = HPB2_ADDR +
        ((ID_SERCOM0 - ID_PAC2) << 10) + SERCOM_USART_INTFLAG_OFFSET;
    init_arm(&arm);

    SERCOM_USART_Type *usart = &arm.mem.sercom[0].USART;
    usart->CTRLA.bit.MODE = SERCOM_USART_CTRLA_MODE_USART_INT_CLK_Val;
    usart->STATUS.bit.TXE = true;
    arm_mem_update_pending(&arm);
    CHECK(!usart->INTFLAG.bit.ERROR,
          "USART TXE status is not reported as an error");
    CHECK(!(arm_mem_load_word(&arm, status_address) &
            (SERCOM_USART_STATUS_TXE << 16)),
          "USART TXE status always reads as zero");

    usart->STATUS.bit.FERR = true;
    arm_mem_update_pending(&arm);
    CHECK(usart->INTFLAG.bit.ERROR,
          "USART frame errors raise the aggregate error flag");
    usart->STATUS.bit.FERR = false;

    arm_mem_store_word(&arm, SERCOM_USART_STATUS_TXE << 16, status_address);
    CHECK(!usart->STATUS.bit.TXE,
          "writing one clears the USART TXE status bit");

    destroy_arm(&arm);
}

static void test_spi_transmit_shift_register(void) {
    arm_t arm;
    uint32_t response;
    const uint32_t data_address = HPB2_ADDR +
        ((ID_SERCOM0 - ID_PAC2) << 10) + SERCOM_SPI_DATA_OFFSET;
    init_arm(&arm);

    SERCOM_SPI_Type *spi = &arm.mem.sercom[0].SPI;
    spi->CTRLA.bit.ENABLE = true;
    spi->CTRLA.bit.MODE = SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val;
    spi->CTRLB.bit.RXEN = true;
    spi->CTRLB.bit.PLOADEN = true;
    spi->CTRLB.bit.SSDE = true;

    arm_mem_store_word(&arm, UINT32_C(0xC3), data_address);
    CHECK(spi->BUFFER[1].bit.VLD && !spi->BUFFER[0].bit.VLD,
          "PLOADEN writes the first byte directly to the shift register");
    arm_mem_spi_sel(&arm, 0, true);
    CHECK(arm_mem_spi_peek(&arm, 0, &response) == 8 && response == UINT32_C(0xC3),
          "SPI slave shifts out a preloaded transmit byte");

    arm_mem_spi_xfer(&arm, 0, UINT32_C(0xA5));
    CHECK(arm_mem_load_word(&arm, data_address) == UINT32_C(0xA5),
          "SPI slave queues MOSI independently of its transmit shift register");
    arm_mem_spi_sel(&arm, 0, false);
    CHECK(!spi->BUFFER[1].bit.VLD &&
              spi->BUFFER[1].bit.DATA == UINT32_C(0xA5),
          "SPI deselection empties the shift register without losing its data");
    arm_mem_spi_sel(&arm, 0, true);
    CHECK(arm_mem_spi_peek(&arm, 0, &response) == 8 && response == UINT32_C(0xA5),
          "SPI underrun transmits the character received in the preceding frame");

    destroy_arm(&arm);
}

static void test_spi_preload_after_selection(void) {
    arm_t arm;
    uint32_t response;
    const uint32_t data_address = HPB2_ADDR +
        ((ID_SERCOM0 - ID_PAC2) << 10) + SERCOM_SPI_DATA_OFFSET;
    init_arm(&arm);

    SERCOM_SPI_Type *spi = &arm.mem.sercom[0].SPI;
    spi->CTRLA.bit.ENABLE = true;
    spi->CTRLA.bit.MODE = SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val;
    spi->CTRLB.bit.PLOADEN = true;
    spi->CTRLB.bit.SSDE = true;

    arm_mem_spi_sel(&arm, 0, true);
    arm_mem_store_word(&arm, UINT32_C(0x42), data_address);
    CHECK(spi->SS && spi->BUFFER[1].bit.VLD && !spi->BUFFER[0].bit.VLD,
          "PLOADEN preloads an empty shift register after slave selection");
    CHECK(arm_mem_spi_peek(&arm, 0, &response) == 8 &&
              response == UINT32_C(0x42),
          "a response preloaded after SSL is visible in the current transaction");

    destroy_arm(&arm);
}

static void test_spi_transmit_with_receiver_disabled(void) {
    arm_t arm;
    uint32_t response;
    const uint32_t sercom_address = HPB2_ADDR +
        ((ID_SERCOM0 - ID_PAC2) << 10);
    const uint32_t ctrlb_address = sercom_address + SERCOM_SPI_CTRLB_OFFSET;
    const uint32_t data_address = sercom_address + SERCOM_SPI_DATA_OFFSET;
    init_arm(&arm);

    SERCOM_SPI_Type *spi = &arm.mem.sercom[0].SPI;
    spi->CTRLA.bit.ENABLE = true;
    spi->CTRLA.bit.MODE = SERCOM_SPI_CTRLA_MODE_SPI_SLAVE_Val;
    spi->CTRLB.bit.RXEN = true;
    spi->CTRLB.bit.PLOADEN = true;
    spi->CTRLB.bit.SSDE = true;

    arm_mem_store_word(&arm, UINT32_C(0xC3), data_address);
    arm_mem_spi_sel(&arm, 0, true);
    CHECK(arm_mem_spi_peek(&arm, 0, &response) == 8 &&
              response == UINT32_C(0xC3),
          "SPI client starts transmitting while its receiver is enabled");

    arm_mem_store_word(&arm,
                       spi->CTRLB.reg & ~SERCOM_SPI_CTRLB_RXEN,
                       ctrlb_address);
    CHECK(!spi->CTRLB.bit.RXEN && !spi->SS,
          "disabling SPI reception rearms the next slave selection");

    arm_mem_store_word(&arm, UINT32_C(0x55), data_address);
    arm_mem_spi_xfer(&arm, 0, UINT32_C(0xA5));
    CHECK(!spi->INTFLAG.bit.RXC &&
              !spi->BUFFER[2].bit.VLD && !spi->BUFFER[3].bit.VLD,
          "disabled SPI receiver discards incoming characters");
    CHECK(arm_mem_spi_peek(&arm, 0, &response) == 8 &&
              response == UINT32_C(0x55),
          "SPI client continues transmitting with its receiver disabled");

    destroy_arm(&arm);
}

static bool bootloader_spi_ready(const arm_t *arm) {
    const SERCOM_SPI_Type *spi = &arm->mem.sercom[0].SPI;
    return spi->CTRLA.bit.ENABLE && spi->CTRLB.bit.SSDE;
}

static void test_threaded_spi_event_handoff(void) {
    arm_t *arm = arm_create();
    uint32_t response = UINT32_MAX;
    CHECK(arm != NULL, "ARM instance initializes for threaded SPI handoff");
    if (!arm) {
        return;
    }

    CHECK(wait_for_arm_condition(arm, bootloader_spi_ready),
          "bundled bootloader initializes its SPI slave");

    arm_spi_sel(arm, true);
    sync_enter(&arm->sync);
    CHECK(!arm->mem.sercom[0].SPI.INTFLAG.bit.SSL,
          "threaded SPI selection hands SSL to waiting firmware");
    sync_leave(&arm->sync);

    CHECK(arm_spi_peek(arm, &response) == 8,
          "threaded SPI transaction exposes a response byte");
    (void)arm_spi_xfer(arm, UINT32_C(0xA5), &response);
    sync_enter(&arm->sync);
    CHECK(!arm->mem.sercom[0].SPI.INTFLAG.bit.RXC,
          "threaded SPI frame hands RXC to waiting firmware");
    sync_leave(&arm->sync);

    arm_spi_sel(arm, false);
    sync_enter(&arm->sync);
    CHECK(!arm->mem.sercom[0].SPI.INTFLAG.bit.TXC,
          "threaded SPI deselection hands TXC to waiting firmware");
    CHECK(!arm->mem.sercom[0].SPI.BUFFER[2].bit.VLD &&
              !arm->mem.sercom[0].SPI.BUFFER[3].bit.VLD,
          "threaded SPI handoff leaves no unread receive character");
    sync_leave(&arm->sync);

    arm_destroy(arm);
}

static bool arm_left_power_down(const arm_t *arm) {
    return arm->mem.pm.RCAUSE.reg == PM_RCAUSE_EXT &&
           !(arm->cpu.pm &&
             arm->mem.pm.SLEEP.bit.IDLE == PM_SLEEP_IDLE_APB_Val &&
             (arm->cpu.scb.scr & SCB_SCR_SLEEPDEEP_Msk));
}

static void test_spi_selection_during_power_down(void) {
    arm_t *arm = arm_create();
    CHECK(arm != NULL, "ARM instance initializes for power-down restart");
    if (!arm) {
        return;
    }

    sync_enter(&arm->sync);
    arm->cpu.pc = UINT32_C(0x102);
    arm->cpu.wfi = false;
    arm->cpu.pm = true;
    arm->cpu.scb.scr = SCB_SCR_SLEEPDEEP_Msk;
    arm->mem.pm.SLEEP.bit.IDLE = PM_SLEEP_IDLE_APB_Val;
    arm->mem.pm.RCAUSE.reg = 0;
    sync_sleep(&arm->sync);
    sync_leave(&arm->sync);

    arm_spi_sel(arm, true);
    CHECK(wait_for_arm_condition(arm, arm_left_power_down),
          "SPI selection resets a coprocessor entering deep sleep before WFI");
    CHECK(wait_for_arm_condition(arm, bootloader_spi_ready),
          "power-down selection restarts the bundled bootloader");

    arm_destroy(arm);
}

static bool arm_test_program_ran(const arm_t *arm) {
    return arm->cpu.r0 == 1;
}

static void test_uart_dequeue_wakes_thread(void) {
    arm_t *arm = arm_create();
    uint8_t value = 0;
    CHECK(arm != NULL, "ARM instance initializes for UART wake test");
    if (!arm) {
        return;
    }

    sync_enter(&arm->sync);
    spsc_queue_clear(&arm->usart[1]);
    arm->mem.nvm[UINT32_C(0x100) >> 2] =
        UINT32_C(0xE7FE2001); /* MOVS r0, #1; B . */
    arm->cpu.pc = UINT32_C(0x102);
    arm->cpu.r0 = 0;
    CHECK(spsc_queue_enqueue(&arm->usart[1], UINT8_C('A')),
          "UART output queue accepts a byte before sleeping");
    CHECK(spsc_queue_flush(&arm->usart[1]),
          "UART output queue publishes a byte before sleeping");
    sync_sleep(&arm->sync);
    sync_leave(&arm->sync);

    CHECK(arm_usart_recv(arm, &value) && value == UINT8_C('A'),
          "UART dequeue returns the queued coprocessor byte");
    CHECK(wait_for_arm_condition(arm, arm_test_program_ran),
          "UART dequeue wakes the sleeping coprocessor thread");

    arm_destroy(arm);
}

static void test_spsc_queue(void) {
    spsc_queue_t queue;
    CHECK(spsc_queue_init(&queue), "queue initializes");

    for (spsc_queue_entry_t entry = 0; entry != SPSC_QUEUE_SIZE + 1; ++entry) {
        CHECK(spsc_queue_enqueue(&queue, entry), "queue accepts an entry");
    }
    CHECK(!spsc_queue_enqueue(&queue, SPSC_QUEUE_SIZE + 1),
          "queue reports full without discarding the pending entry");

    for (spsc_queue_entry_t entry = 0; entry != SPSC_QUEUE_SIZE; ++entry) {
        CHECK(spsc_queue_dequeue(&queue) == entry, "queue preserves FIFO order");
        CHECK(spsc_queue_flush(&queue), "queue flushes the retained entry when space opens");
    }
    CHECK(spsc_queue_dequeue(&queue) == SPSC_QUEUE_SIZE,
          "queue retains an entry submitted while full");
    CHECK(spsc_queue_dequeue(&queue) == SPSC_QUEUE_INVALID_ENTRY,
          "queue is empty after all entries are consumed");
    spsc_queue_destroy(&queue);
}

static void test_bundled_bootloader(void) {
    arm_t arm;
    init_arm(&arm);

    const uint8_t *flash = (const uint8_t *)arm.mem.nvm;
    const uint32_t *vectors = arm.mem.nvm;
    CHECK(sizeof(cemu_free_bootloader) <= UINT32_C(0x2000),
          "bundled bootloader fits in the SAMD21 8 KiB boot region");
    CHECK(memcmp(flash, cemu_free_bootloader, sizeof(cemu_free_bootloader)) == 0,
          "empty ARM flash is initialized with the bundled bootloader");
    CHECK(vectors[0] == HMCRAMC0_ADDR + HMCRAMC0_SIZE - sizeof(uint32_t),
          "bundled bootloader reserves the double-tap word below its stack");
    CHECK((vectors[1] & 1u) && vectors[1] < UINT32_C(0x2000),
          "bundled bootloader reset vector points inside the boot region");
    CHECK(flash[UINT32_C(0x2000)] == UINT8_C(0xFF),
          "bundled bootloader leaves application flash erased");

    destroy_arm(&arm);
}

static void test_bundled_bootloader_double_tap_reset(void) {
    static const uint32_t app_start = UINT32_C(0x2000);
    static const uint32_t double_tap_address =
        HMCRAMC0_ADDR + HMCRAMC0_SIZE - sizeof(uint32_t);
    static const uint32_t double_tap_magic = UINT32_C(0xF01669EF);
    arm_t arm;
    init_arm(&arm);

    /* Install a valid application vector and execute its SYSRESETREQ store. */
    arm.mem.nvm[app_start >> 2] = double_tap_address;
    arm.mem.nvm[(app_start >> 2) + 1] = app_start + 9;
    arm.mem.nvm[(app_start >> 2) + 2] = UINT32_C(0xBF0060DA); /* STR r2, [r3, #12] */
    arm.mem.ram[(HMCRAMC0_SIZE >> 2) - 1] = double_tap_magic;
    arm.cpu.pc = app_start + 10;
    arm.cpu.r2 = UINT32_C(0x05FA0004);
    arm.cpu.r3 = SCB_BASE;
    arm_cpu_execute(&arm);

    CHECK(arm.mem.pm.RCAUSE.reg == PM_RCAUSE_SYST,
          "SYSRESETREQ records a system reset cause");
    CHECK(arm.cpu.pc == arm.mem.nvm[1] + 1,
          "SYSRESETREQ restarts at the bootloader reset vector");
    CHECK(arm.mem.ram[(HMCRAMC0_SIZE >> 2) - 1] == double_tap_magic,
          "SYSRESETREQ preserves the double-tap marker in SRAM");

    bool ready = false;
    for (unsigned int instruction = 0; !ready && instruction != 10000; ++instruction) {
        arm_cpu_execute(&arm);
        ready = arm.mem.sercom[0].SPI.CTRLA.bit.ENABLE &&
                arm.mem.sercom[3].USART.CTRLA.bit.ENABLE;
    }
    CHECK(ready, "double-tap reset keeps the bootloader active");
    CHECK(arm.cpu.pc < app_start,
          "double-tap reset does not hand control back to the application");
    CHECK(arm.mem.ram[(HMCRAMC0_SIZE >> 2) - 1] == 0,
          "bootloader consumes the double-tap marker");

    destroy_arm(&arm);
}

static void arm_flash_store_byte(arm_t *arm, uint32_t address, uint8_t value) {
    sync_enter(&arm->sync);
    ((uint8_t *)arm->mem.nvm)[address] = value;
    sync_leave(&arm->sync);
}

static uint8_t arm_flash_load_byte(arm_t *arm, uint32_t address) {
    uint8_t value;
    sync_enter(&arm->sync);
    value = ((const uint8_t *)arm->mem.nvm)[address];
    sync_leave(&arm->sync);
    return value;
}

static void test_arm_flash_serialization(void) {
    arm_t *arm = arm_create();
    FILE *image = tmpfile();
    CHECK(arm != NULL, "ARM instance initializes for flash serialization");
    CHECK(image != NULL, "temporary ARM flash image opens");
    if (!arm || !image) {
        arm_destroy(arm);
        if (image) {
            fclose(image);
        }
        return;
    }

    arm_flash_store_byte(arm, UINT32_C(0x2345), UINT8_C(0xA5));
    CHECK(arm_save_flash(arm, image), "ARM flash serializes completely");
    CHECK(ftell(image) == FLASH_SIZE, "serialized ARM flash has the expected size");
    arm_flash_store_byte(arm, UINT32_C(0x2345), UINT8_C(0x5A));
    rewind(image);
    CHECK(arm_restore_flash(arm, image), "ARM flash restores completely");
    CHECK(arm_flash_load_byte(arm, UINT32_C(0x2345)) == UINT8_C(0xA5),
          "ARM flash restore reproduces saved bytes exactly");

    fclose(image);
    arm_destroy(arm);
}

static void test_arm_bootloader_info(void) {
    static const char old_free_info[] =
        "UF2 Bootloader CEmu free 0.9\r\n"
        "Model: TI-Python compatible\r\n"
        "Board-ID: TI Python\r\n";
    static const char ti_info[] =
        "UF2 Bootloader v1.1.1S SFRO\r\n"
        "Model: TI-Python\r\n"
        "Board-ID: TI Python\r\n";
    arm_t *arm = arm_create();
    char description[ARM_BOOTLOADER_DESCRIPTION_SIZE];
    CHECK(arm != NULL, "ARM instance initializes for bootloader identification");
    if (!arm) {
        return;
    }

    CHECK(arm_get_bootloader_info(arm, description, sizeof(description)) ==
              ARM_BOOTLOADER_CEMU_FREE &&
              strcmp(description, "CEmu free 1.0") == 0,
          "bundled ARM flash identifies the CEmu free bootloader");

    sync_enter(&arm->sync);
    memset(arm->mem.nvm, UINT8_C(0xFF), UINT32_C(0x2000));
    memcpy((uint8_t *)arm->mem.nvm + UINT32_C(0x120), old_free_info,
           sizeof(old_free_info) - 1);
    sync_leave(&arm->sync);
    CHECK(arm_get_bootloader_info(arm, description, sizeof(description)) ==
              ARM_BOOTLOADER_CEMU_FREE &&
              strcmp(description, "CEmu free 0.9") == 0,
          "saved ARM flash reports its older CEmu free bootloader version");

    sync_enter(&arm->sync);
    memset(arm->mem.nvm, UINT8_C(0xFF), UINT32_C(0x2000));
    memcpy((uint8_t *)arm->mem.nvm + UINT32_C(0x120), ti_info,
           sizeof(ti_info) - 1);
    sync_leave(&arm->sync);
    CHECK(arm_get_bootloader_info(arm, description, sizeof(description)) ==
              ARM_BOOTLOADER_TI_UF2 &&
              strcmp(description, "TI UF2 v1.1.1S SFRO") == 0,
          "TI INFO_UF2 metadata identifies and versions its bootloader");

    sync_enter(&arm->sync);
    ((uint8_t *)arm->mem.nvm)[UINT32_C(0x120) + sizeof(ti_info) - 3] = 'X';
    sync_leave(&arm->sync);
    CHECK(arm_get_bootloader_info(arm, description, sizeof(description)) ==
              ARM_BOOTLOADER_UNKNOWN &&
              strcmp(description, "unknown/custom") == 0,
          "incomplete TI metadata does not identify a custom bootloader as TI");

    arm_destroy(arm);
}

static void test_arm_cycle_throttle(void) {
    arm_t *arm = arm_create();
    CHECK(arm != NULL, "ARM instance initializes for cycle throttling");
    if (!arm) {
        return;
    }

    const uint64_t target = arm_get_time(arm) + UINT64_C(4096);
    arm_run_until(arm, target);
    const uint64_t reached = arm_get_time(arm);
    /* Instructions are atomic; POP {r0-r7, pc} is the longest at 12 cycles. */
    CHECK(reached >= target && reached <= target + 11,
          "ARM worker stops within one instruction of its virtual cycle budget");
    for (unsigned int attempt = 0; attempt != 1000; ++attempt) {
        thrd_yield();
    }
    CHECK(arm_get_time(arm) == reached,
          "ARM worker remains throttled after reaching its cycle budget");

    arm_advance_to(arm, target + UINT64_C(1000000));
    arm_pause(arm);
    const uint64_t paused = arm_get_time(arm);
    for (unsigned int attempt = 0; attempt != 1000; ++attempt) {
        thrd_yield();
    }
    CHECK(arm_get_time(arm) == paused,
          "explicit pause synchronously stops an in-flight ARM budget");

    arm_destroy(arm);
}

static void test_bundled_bootloader_identification(void) {
    arm_t *arm = arm_create();
    CHECK(arm != NULL, "ARM instance initializes for bootloader identification");
    if (!arm) {
        return;
    }

    static const uint8_t expected[] = { 'P', 'O', 'B' };
    uint8_t received[sizeof(expected)];
    size_t count = 0;
    bool ready = false;
    for (unsigned int attempt = 0; !ready && attempt != 1000000; ++attempt) {
        sync_enter(&arm->sync);
        ready = arm->mem.sercom[3].USART.CTRLA.bit.ENABLE &&
                arm->mem.sercom[3].USART.CTRLB.bit.RXEN;
        sync_leave(&arm->sync);
        if (!ready) {
            arm_run_until(arm, arm_get_time(arm) + 256);
        }
    }
    CHECK(ready, "bundled bootloader initializes its UART");
    CHECK(arm_usart_send(arm, UINT8_C(0x14)),
          "bootloader identification request enters the UART");
    for (unsigned int attempt = 0; count < sizeof(received) && attempt != 1000000; ++attempt) {
        uint8_t value;
        if (arm_usart_recv(arm, &value)) {
            received[count++] = value;
        } else {
            arm_run_until(arm, arm_get_time(arm) + 256);
        }
    }
    CHECK(count == sizeof(expected), "bundled bootloader answers the UART request");
    CHECK(count != sizeof(expected) || memcmp(received, expected, sizeof(expected)) == 0,
          "bundled bootloader identifies itself as POB");
    arm_destroy(arm);
}

static void write_coproc_header(FILE *image, uint8_t present) {
    static const uint8_t magic[] = { 'C', 'A', 'R', 'M' };
    (void)fwrite(magic, sizeof(magic), 1, image);
    (void)fwrite(&present, sizeof(present), 1, image);
}

static void test_coproc_reset_rebases_clock(void) {
    asic.python = true;
    coproc_reset();
    CHECK(coproc.arm != NULL, "Python ASIC creates an ARM for reset timing");
    if (!coproc.arm) {
        return;
    }

    arm_run_until(coproc.arm, UINT64_C(4096));
    coproc_reset();
    CHECK(arm_get_time(coproc.arm) == 0,
          "ASIC reset rebases the ARM to the scheduler clock epoch");
    coproc_free();
}

static void test_coproc_state_serialization(void) {
    FILE *image = tmpfile();
    asic.python = true;
    coproc_reset();
    CHECK(coproc.arm != NULL, "Python ASIC creates its ARM coprocessor");
    CHECK(image != NULL, "temporary coprocessor state image opens");
    if (!coproc.arm || !image) {
        coproc_free();
        if (image) {
            fclose(image);
        }
        return;
    }

    arm_flash_store_byte(coproc.arm, UINT32_C(0x3456), UINT8_C(0xC3));
    CHECK(coproc_save(image), "coprocessor state serializes");
    CHECK(ftell(image) > (long)(5u + FLASH_SIZE),
          "coprocessor state contains complete ARM flash and execution state");
    arm_flash_store_byte(coproc.arm, UINT32_C(0x3456), UINT8_C(0x3C));
    rewind(image);
    CHECK(coproc_restore(image), "coprocessor state restores");
    CHECK(arm_flash_load_byte(coproc.arm, UINT32_C(0x3456)) == UINT8_C(0xC3),
          "coprocessor restore reproduces saved ARM flash exactly");
    fclose(image);

    arm_t *original = coproc.arm;
    image = tmpfile();
    CHECK(image != NULL, "temporary malformed coprocessor image opens");
    if (image) {
        write_coproc_header(image, 1);
        (void)fputc(0, image);
        rewind(image);
        CHECK(!coproc_restore(image), "truncated coprocessor state is rejected");
        CHECK(coproc.arm == original,
              "failed coprocessor restore preserves the running instance");
        CHECK(arm_flash_load_byte(coproc.arm, UINT32_C(0x3456)) == UINT8_C(0xC3),
              "failed coprocessor restore preserves existing flash");
        fclose(image);
    }

    image = tmpfile();
    CHECK(image != NULL, "temporary absent-coprocessor image opens");
    if (image) {
        write_coproc_header(image, 0);
        rewind(image);
        CHECK(!coproc_restore(image),
              "Python state without a coprocessor is rejected");
        CHECK(coproc.arm == original,
              "rejected Python state preserves the running coprocessor");
        fclose(image);
    }

    asic.python = false;
    image = tmpfile();
    CHECK(image != NULL, "temporary non-Python coprocessor image opens");
    if (image) {
        write_coproc_header(image, 0);
        rewind(image);
        CHECK(coproc_restore(image), "non-Python state accepts an absent coprocessor");
        CHECK(coproc.arm == NULL, "non-Python restore removes the coprocessor");
        fclose(image);
    }

    image = tmpfile();
    CHECK(image != NULL, "temporary non-Python state image opens");
    if (image) {
        CHECK(coproc_save(image), "non-Python coprocessor state serializes");
        CHECK(ftell(image) == 5, "absent coprocessor state stores only its header");
        rewind(image);
        CHECK(coproc_restore(image), "serialized non-Python coprocessor state restores");
        fclose(image);
    }
    coproc_free();
}

int main(void) {
    test_adc_flags();
    test_sbc_flags();
    test_random_adc_sbc_flags();
    test_relative_branches();
    test_instruction_cycle_counts();
    test_high_exception_return();
    test_core_register_reset_values();
    test_exception_priorities();
    test_svc_pending_register();
    test_svc_instruction_pending();
    test_peripheral_reset();
    test_sercom_status_polling();
    test_usart_transmitter_empty_status();
    test_spi_transmit_shift_register();
    test_spi_preload_after_selection();
    test_spi_transmit_with_receiver_disabled();
    test_threaded_spi_event_handoff();
    test_spi_selection_during_power_down();
    test_uart_dequeue_wakes_thread();
    test_spsc_queue();
    test_bundled_bootloader();
    test_bundled_bootloader_double_tap_reset();
    test_arm_flash_serialization();
    test_arm_bootloader_info();
    test_arm_cycle_throttle();
    test_bundled_bootloader_identification();
    test_coproc_reset_rebases_clock();
    test_coproc_state_serialization();

    if (failures) {
        fprintf(stderr, "%u ARM emulation test(s) failed\n", failures);
        return 1;
    }
    puts("All ARM emulation tests passed");
    return 0;
}
