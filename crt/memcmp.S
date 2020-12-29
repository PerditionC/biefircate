	.globl	memcmp
memcmp:
	pushq	%rsi			/* preserve %rsi & %rdi */
	pushq	%rdi
	movq	%rcx, %rsi		/* load registers for a comparison */
	movq	%rdx, %rdi
	movq	%r8, %rcx
	xorl	%eax, %eax
	jrcxz	9f			/* if nothing to compare, return 0 */
	repz
	cmpsb
	jz	9f			/* ZF = 1 => return 0 */
	sbbl	%eax, %eax		/* CF = 1 => we should return < 0 */
	shll	%eax			/* CF = 0 => we should return > 0 */
	incl	%eax
9:
	popq	%rdi
	popq	%rsi
	retq
