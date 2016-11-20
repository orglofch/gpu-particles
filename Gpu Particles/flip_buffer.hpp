#ifndef _FLIP_BUFFER_
#define _FLIP_BUFFER_

#include "Utility\gl.hpp"

/**
 * A buffer which can be flipped back and forth, where one side
 * is used as input and another is written too.
 */
class FlipBuffer
{
public:
	FlipBuffer() {
		buffers[0] = 0;
		buffers[1] = 0;
	}

	void flip() {
		active_buffer = !active_buffer;
	}

	GLuint *getActiveBuffer() {
		return buffers + active_buffer;
	}

	GLuint *getInactiveBuffer() {
		return buffers + !active_buffer;
	}

	GLuint *getBuffers() {
		return buffers;
	}

private:
	int active_buffer = 0;

	GLuint buffers[2];
};

#endif