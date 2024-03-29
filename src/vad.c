#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "pav_analysis.h"
#include "vad.h"

const float FRAME_TIME = 10.0F; /* in ms. */

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only this labels are needed. You need to add all labels, in case
 * you want to print the internal state in string format
 */

const char *state_str[] = {
  "UNDEF", "S", "V", "INIT"
};

const char *state2str(VAD_STATE st) {
  return state_str[st];
}

/* Define a datatype with interesting features */
typedef struct {
  float zcr;
  float p;
  float am;
} Features;

/* 
 * TODO: Delete and use your own features!
 */

Features compute_features(const float *x, int N) {
  /*
   * Input: x[i] : i=0 .... N-1 
   * Ouput: computed features
   */
  /* 
   * DELETE and include a call to your own functions
   *
   * For the moment, compute random value between 0 and 1 
   */
  Features feat;
  feat.p = compute_power(x,N);
  return feat;
}

/* 
 * TODO: Init the values of vad_data
 */

VAD_DATA * vad_open(float rate) {
  VAD_DATA *vad_data = malloc(sizeof(VAD_DATA));
  vad_data->state = ST_INIT;
  vad_data->sampling_rate = rate;
  vad_data->frame_length = rate * FRAME_TIME * 1e-3;
  return vad_data;
}

VAD_STATE vad_close(VAD_DATA *vad_data) {
    // Store the current state and last known state
    VAD_STATE last_known_state = vad_data->last_st_known;
    VAD_STATE next_state;

    // If the last known state is undefined, wait until a valid state is received
    if (last_known_state == ST_UNDEF) {
        while (1) {
            // Wait for the next state
            next_state = vad_data->state;

            // If the next state is voice or silence, break the loop
            if (next_state == ST_VOICE || next_state == ST_SILENCE) {
                break;
            }
        }

        // Update all the undefined states to the next state
        vad_data->state = next_state;
        last_known_state = next_state;
    }

    // Free the memory allocated for the VAD_DATA structure
    free(vad_data);

    // Return the last known state
    return last_known_state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * TODO: Implement the Voice Activity Detection 
 * using a Finite State Automata
 */

VAD_STATE vad(VAD_DATA *vad_data, float *x, float alpha1, float alpha2) {
    // Compute features of the input audio frame
    Features f = compute_features(x, vad_data->frame_length);

    // Update the VAD state based on the current state and computed features
    switch (vad_data->state) {
        case ST_INIT:
            // Initialize VAD state and save initial feature
            vad_data->state = ST_SILENCE;
            vad_data->p0 = f.p;
            vad_data->last_st_known = ST_SILENCE;
            break;

        case ST_SILENCE:
            // Check if the feature exceeds the silence threshold
            if (f.p > vad_data->p0 + alpha1) {
                vad_data->state = ST_VOICE;  // Transition to voice state
                vad_data->last_st_known = ST_SILENCE;
            }
            break;

        case ST_VOICE:
            // Check if the feature falls below the voice threshold
            if (f.p < vad_data->p0 + alpha2) {
                vad_data->state = ST_SILENCE;  // Transition to silence state
                vad_data->last_st_known = ST_VOICE;
            }
            break;

        case ST_UNDEF:
            // Check for transition from silence to voice
            if (f.p > vad_data->p0 + alpha1) {
                vad_data->state = ST_VOICE;  // Transition to voice state
                vad_data->undef_count = 0;
                vad_data->last_st_known = ST_VOICE;
            } else if (f.p < vad_data->p0 + alpha2) { // Check for transition from voice to silence
                vad_data->state = ST_SILENCE;  // Transition to silence state
                vad_data->undef_count = 0;
                vad_data->last_st_known = ST_SILENCE;
            }
            break;
    }

    // Return the current state
    return vad_data->state;
}

void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}