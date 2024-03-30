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
VAD_STATE vad(VAD_DATA *vad_data, float *x, float alpha1, float alpha2) {
  // Compute features of the input audio frame
  Features f = compute_features(x, vad_data->frame_length);

  // Update the VAD state based on the current state and computed features
  switch (vad_data->state) {
    case ST_INIT:
      // Inicialitza l'estat VAD i guarda la característica inicial
      vad_data->state = ST_SILENCE;
      vad_data->p0 = f.p;
      vad_data->last_st_known = ST_SILENCE;
      break;

    case ST_SILENCE:
      // Comprova si supera el llindar de silenci
      if (f.p > vad_data->p0 + alpha1) {
/*!!!!*/vad_data->state = ST_VOICE;  // Transició a l'estat undef 
        vad_data->undef_count = 0;
        vad_data->last_st_known = ST_SILENCE;
      }
      break;

    case ST_VOICE:
      // Comprova si no supera del llindar de veu
      if (f.p < vad_data->p0 + alpha2) {
/*!!!!*/vad_data->state = ST_SILENCE;  // Transició a l'estat undef
        vad_data->undef_count = 0;
        vad_data->last_st_known = ST_VOICE;
      }
      break;

    case ST_UNDEF:
      if (f.p > vad_data->p0 + alpha1 && vad_data->last_st_known == ST_SILENCE) { 
        // Si supera el llindar de silenci i l'últim estat conegut era silenci
        if (vad_data->undef_count >= 3) {
          // Si hi ha 3 o més estats undef consecutius que superen el llindar de silenci, canvia a veu
          vad_data->state = ST_VOICE;
          vad_data->last_st_known = ST_VOICE;
        } else {
          // Sinó, actualitza el comptador
          vad_data->undef_count++;
        }
      } else if (f.p < vad_data->p0 + alpha2 && vad_data->last_st_known == ST_VOICE) {
        // Si no supera el llindar de veu i l'últim estat conegut era veu
        if (vad_data->undef_count >= 3) {
          // Si hi ha 3 o més estats undef consecutius que no superen el llindar de veu, canvia a silenci
          vad_data->state = ST_SILENCE;
          vad_data->last_st_known = ST_SILENCE;
        } else {
          // Sinó, actualitza el comptador
          vad_data->undef_count++;
        }
      } else {
        // Cap dels casos anteriors -> surt de l'estat undef i reinicia el comptador
        vad_data->state = vad_data->last_st_known;
        vad_data->undef_count = 0;
      }
      // A partir d'aquí és per les últimes trames (en cas que acabés amb estat undef)
      // Hi ha un contador que conta el temps de 5 trames. Si passat aquest temps una
      // trama no ha sortit del estat undef, la considerem com si fos una trama final.
      if (vad_data->state == ST_UNDEF) {
        // Inicialitza el temps total de processament per a aquesta trama indefinida
      float total_processing_time = 0.0;

      // Bucle fins que s'obté un estat vàlid (veu o silenci) o s'excedeix la durada màxima
      while (1) {
        total_processing_time += FRAME_TIME / 1000.0;  

        if (total_processing_time >= MAX_DURATION) {
          // Si excedeix la durada màxima, canvia l'estat de les trames indefinides a silenci
          vad_data->state = ST_SILENCE;
          break;  
        }

        VAD_STATE next_state = vad_data->state;

        // Si l'estat següent és veu o silenci, assigna'l a l'estat indefinit i surt del bucle
        if (next_state == ST_VOICE || next_state == ST_SILENCE) {
          vad_data->state = next_state;
          break;
        }
      }
    }
    break;
  }

  return vad_data->state;  //Unicament retorna ST_UNDEF si ha expirat el temporitzador
}


void vad_show_state(const VAD_DATA *vad_data, FILE *out) {
  fprintf(out, "%d\t%f\n", vad_data->state, vad_data->last_feature);
}

