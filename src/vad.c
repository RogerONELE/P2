#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "pav_analysis.h"
#include "vad.h"

const float FRAME_TIME = 10.0F; /* in ms. */
const float MAX_DURATION = FRAME_TIME * 3.0F; /* Maximum duration in ms for an undefined state */

/* 
 * As the output state is only ST_VOICE, ST_SILENCE, or ST_UNDEF,
 * only these labels are needed. You need to add all labels, in case
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
  /* 
   * Handle the last undecided frames
   */
  VAD_STATE state = vad_data->state;
  // Si aqui arriba un ST_UNDEFINED, 
  // significa que ja son els ultims i no tenen opció de canviar
  if (state == ST_UNDEF) {
    state = ST_SILENCE; //Els declarem com silenci
  }
  free(vad_data);
  return state;
}

unsigned int vad_frame_size(VAD_DATA *vad_data) {
  return vad_data->frame_length;
}

/* 
 * Implement the Voice Activity Detection 
 * using a Finite State Automata
 */
VAD_STATE vad(VAD_DATA *vad_data, float *x, float alpha1, float alpha2, float delta) {
  // Compute features of the input audio frame
  Features f = compute_features(x, vad_data->frame_length);

  // Update the VAD state based on the current state and computed features
  switch (vad_data->state) {
    case ST_INIT:
      // Inicialitza l'estat VAD i guarda la característica inicial
      vad_data->state = ST_SILENCE;
      vad_data->p0 = f.p;
      vad_data->last_state_known = ST_SILENCE;
      break;

    case ST_SILENCE:
      // Comprova si supera el llindar de veu
      if (f.p > vad_data->p0 + alpha1) {
/*!!!!*/vad_data->state = ST_UNDEF;  // Transició a l'estat undef 
        vad_data->undef_count = 0;
        vad_data->last_state_known = ST_SILENCE;
      }
      break;

    case ST_VOICE:
      // Comprova si no supera del llindar de silenci
      if (f.p < vad_data->p0 + alpha2) {
/*!!!!*/vad_data->state = ST_UNDEF;  // Transició a l'estat undef
        vad_data->undef_count = 0;
        vad_data->last_state_known = ST_VOICE;
      }
      break;

    case ST_UNDEF:

    //Comprovar si es SILENCI ---> VEU
      if (f.p > vad_data->p0 + alpha1) { 
        // Si supera el llindar de silenci 
            
        if (vad_data->undef_count < delta +1){
          vad_data->undef_count = vad_data->undef_count +1;
          vad_data->state = ST_UNDEF;
        }  
        else
          vad_data->state = ST_VOICE;
      } 
      else {
           vad_data->state = ST_SILENCE;
          
        } 
    //Comprovar si es  VEU ---> SILENCI
      if (f.p < vad_data->p0 + alpha2) {
        // Si no supera el llindar de veu
        if (vad_data->undef_count < delta+1){
          vad_data->undef_count++;
          vad_data->state = ST_UNDEF;
        }  
        else
          vad_data->state = ST_SILENCE;
      } 
      else {
           vad_data->state = ST_VOICE;
      } 
      break;
  }

  return vad_data->state;  //Unicament retorna ST_UNDEF si ha expirat el temporitzador
}


void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}

