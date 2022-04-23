#ifndef SWEXN_H_
#define SWEXN_H_

void
handle_exn( int *ebp, unsigned int cause, unsigned int cr2 );

#endif /* SWEXN_H_ */
