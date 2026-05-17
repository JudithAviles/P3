PAV - P3: estimación de pitch
=============================

Esta práctica se distribuye a través del repositorio GitHub [Práctica 3](https://github.com/albino-pav/P3).
Siga las instrucciones de la [Práctica 2](https://github.com/albino-pav/P2) para realizar un `fork` de la
misma y distribuir copias locales (*clones*) del mismo a los distintos integrantes del grupo de prácticas.

Recuerde realizar el *pull request* al repositorio original una vez completada la práctica.

Ejercicios básicos
------------------

- Complete el código de los ficheros necesarios para realizar la estimación de pitch usando el programa
  `get_pitch`.

   * Complete el cálculo de la autocorrelación e inserte a continuación el código correspondiente.

     ```cpp
      // Autocorrelation computation (biased)
      if (r[0] == 0.0F) //to avoid log() and divide zero 
        r[0] = 1e-10; 
      void PitchAnalyzer::autocorrelation(const vector<float> &x, vector<float> &r) const {
        for (unsigned int l = 0; l < r.size(); ++l) {
          r[l] = 0;
          int N = x.size();
          for (int n = l; n < N; ++n){
            r[l] += x[n]*x[n-l];
          }
          r[l] = r[l]/N;
        }
        if (r[0] == 0.0F) r[0] = 1e-10; 
      }
     ```
     Formula: r[l] = (1/N) * sum(x[n] * x[n-l]) for n=l to N

   * Inserte una gŕafica donde, en un *subplot*, se vea con claridad la señal temporal de un segmento de
     unos 30 ms de un fonema sonoro y su periodo de pitch; y, en otro *subplot*, se vea con claridad la
	 autocorrelación de la señal y la posición del primer máximo secundario.

	 NOTA: es más que probable que tenga que usar Python, Octave/MATLAB u otro programa semejante para
hacerlo. Se valorará la utilización de la biblioteca matplotlib de Python.

![30ms segment + autocorrelation](prueba_autocorr.png)

  * Determine el mejor candidato para el periodo de pitch localizando el primer máximo secundario de la
    autocorrelación. Inserte a continuación el código correspondiente.

     ```cpp
     // Find maximum autocorrelation in pitch range
     vector<float> r(npitch_max);
     autocorrelation(x, r);
     vector<float>::const_iterator iR = r.begin(), iRMax = r.begin() + npitch_min;
     for(iR = iRMax; (iR < r.begin()+npitch_max-1 && iR < r.end()); iR++){
       if(*iR > *iRMax){
         iRMax = iR;
       }
     }
     unsigned int lag = iRMax - r.begin();
     float f0 = (float) samplingFreq/(float) lag;
     ```

   * Implemente la regla de decisión sonoro o sordo e inserte el código correspondiente.

     ```cpp
     // Unvoiced decision rule
     bool PitchAnalyzer::unvoiced(float pot, float r1norm, float rmaxnorm, float zcr) const {
       // pot: power in dB = 10*log10(r[0])
       // r1norm: r[1]/r[0] - correlation at lag 1
       // rmaxnorm: r[lag_max]/r[0] - correlation at pitch period
       // zcr: zero crossing rate
       
       if (pot < pot_threshold || zcr < zcr_threshold*samplingFreq/2){
         return true;
       } else if (r1norm >= r1norm_threshold && rmaxnorm >= rmaxnorm_threshold){
         return false;
       } else{
         return true;
       }
     }
     ```

   * Puede serle útil seguir las instrucciones contenidas en el documento adjunto `código.pdf`.

- Una vez completados los puntos anteriores, dispondrá de una primera versión del estimador de pitch. El 
  resto del trabajo consiste, básicamente, en obtener las mejores prestaciones posibles con él.

  * Utilice el programa `wavesurfer` para analizar las condiciones apropiadas para determinar si un
    segmento es sonoro o sordo. 
	
	  - Inserte una gráfica con la estimación de pitch incorporada a `wavesurfer` y, junto a ella, los 
	    principales candidatos para determinar la sonoridad de la voz: el nivel de potencia de la señal
		(r[0]), la autocorrelación normalizada de uno (r1norm = r[1] / r[0]) y el valor de la
		autocorrelación en su máximo secundario (rmaxnorm = r[lag] / r[0]).

      ![Wavesurfer parameters](prueba_wavesurfer.png)

      Parámetros: pot (umbral -42 dB), r1norm (umbral 0.48), rmaxnorm (umbral 0.34). Nosotros también hemos considerado
      utilizar la tasa de cruces por cero zcr (umbral 0.012).

	    Puede considerar, también, la conveniencia de usar la tasa de cruces por cero.

	    Recuerde configurar los paneles de datos para que el desplazamiento de ventana sea el adecuado, que
	    en esta práctica es de 15 ms.

    - Use el estimador de pitch implementado en el programa `wavesurfer` en una señal de prueba y compare
	  su resultado con el obtenido por la mejor versión de su propio sistema.  Inserte una gráfica
    ilustrativa del resultado de ambos estimadores.

      ![F0 comparison: our estimator vs Wavesurfer reference](prueba_comparison.png)
      
	  Aunque puede usar el propio Wavesurfer para obtener la representación, se valorará
		el uso de alternativas de mayor calidad (particularmente Python).

      Podemos observar que nuestra estimación es muy cercana a la obtenida por Wavesurfer. Únicamente se detecta tono en los segmentos apropiados y los segmentos sordos son etiquetados apropiadamente, con una pequeña cantidad de error en las secciones de traspaso de sordo a sonoro y de sonoro a sordo. El tono estimado en sí también es muy correcto, aunque comete algún error en los picos y cambios repentinos a causa del postprocesado aplicado (el filtro de mediana y la prevención de errores).
  
  * Optimice los parámetros de su sistema de estimación de pitch e inserte una tabla con las tasas de error
    y el *score* TOTAL proporcionados por `pitch_evaluate` en la evaluación de la base de datos 
	`pitch_db/train`..

    | alpha0 (dB) | alpha1 (r1/r0) | alpha2 (rmax/r0) | alpha3 (zcr) | TOTAL Score |
    |-------------|----------------|-----------------|-----------------|-------------|
    | -42         | 0.48           | 0.34            |0.012            | **91.48%**  |
    | -42         | 0.48           | 0.34            |0.011            | 91.47%      |
    | -43         | 0.48           | 0.34            |0.012            | 91.46%      |
    | -43         | 0.48           | 0.34            |0.011            | 91.45%      |
    | -41         | 0.46           | 0.32            |0.009            | 91.19%      |

    Parámetros optimizados: alpha0=-42, alpha1=0.48, alpha2=0.34, alpha3=0.012.
    Hemos optimizado los parámetros a través del script `grid_search.sh`, el cual nos ha permitido comprobar múltiples combinaciones de diferentes valores para cada parámetro para encontrar la que nos daría el score máximo. Antes de utilizar `grid_search`se ha hecho una búsqueda inicial de valores apropiados manual para reducir la cantidad de valores a buscar en `grid_search` y reducir su tiempo de computación.

Ejercicios de ampliación
------------------------

- Usando la librería `docopt_cpp`, modifique el fichero `get_pitch.cpp` para incorporar los parámetros del
  estimador a los argumentos de la línea de comandos.
  
  Esta técnica le resultará especialmente útil para optimizar los parámetros del estimador. Recuerde que
  una parte importante de la evaluación recaerá en el resultado obtenido en la estimación de pitch en la
  base de datos.

  * Inserte un *pantallazo* en el que se vea el mensaje de ayuda del programa y un ejemplo de utilización
    con los argumentos añadidos.

    ![Pantallazo de `get_pitch -h`](get_pitch -h.png)

    Ejemplo de uso:
    ```bash
    get_pitch --alpha0=-40 --alpha1=0.45 --alpha2=0.35 --alpha3=0.015 --method=cepstrum entrada.wav salida.f0
    ```

- Implemente las técnicas que considere oportunas para optimizar las prestaciones del sistema de estimación
  de pitch.

  Entre las posibles mejoras, puede escoger una o más de las siguientes:

  * Técnicas de preprocesado: filtrado paso bajo, diezmado, *center clipping*, etc.
  * Técnicas de postprocesado: filtro de mediana, *dynamic time warping*, etc.
  * Métodos alternativos a la autocorrelación: procesado cepstral, *average magnitude difference function*
    (AMDF), etc.
  * Optimización **demostrable** de los parámetros que gobiernan el estimador, en concreto, de los que
    gobiernan la decisión sonoro/sordo.
  * Cualquier otra técnica que se le pueda ocurrir o encuentre en la literatura.

  Encontrará más información acerca de estas técnicas en las [Transparencias del Curso](https://atenea.upc.edu/pluginfile.php/2908770/mod_resource/content/3/2b_PS%20Techniques.pdf)
  y en [Spoken Language Processing](https://discovery.upc.edu/iii/encore/record/C__Rb1233593?lang=cat).
  También encontrará más información en los anexos del enunciado de esta práctica.

  Incluya, a continuación, una explicación de las técnicas incorporadas al estimador. Se valorará la
  inclusión de gráficas, tablas, código o cualquier otra cosa que ayude a comprender el trabajo realizado.

También se valorará la realización de un estudio de los parámetros involucrados. Por ejemplo, si se opta
por implementar el filtro de mediana, se valorará el análisis de los resultados obtenidos en función de
la longitud del filtro.

## Técnicas Implementadas

### Preprocesado
- **Filtro paso bajo (LPF)**: Filtro de averaging (3x1) para suavizar la señal
  Para implementar el filtro paso bajo se han probado diferentes tipos de LPF y diferentes tamaños para estos.

  Probamos a implementar un filtro butterworth de orden 4, pero, además de incrementar el coste computacional del programa, no fue muy efectivo, y causaba un aumento en la tasa de segmentos sordos identificados como sonoros.

  Como alternativa, hemos implementado un filtro pasabajo más simple y fácil de optimizar: un filtro de averaging. Hemos probado diferentes tamaños (2x1, 3x1, 5x1) y determinado que 3x1 es el mejor. Con 2x1 no se tiene en cuenta la muestra posterior y acaba causando fine errors. En cambio, con tamaño 5x1 se hace media entre demasiadas muestras y se aplana demasiado la señal. Con 3x1 evitamos estos dos problemas.

- **Normalización**: Ajuste de amplitud al rango [-1, 1]  
  Ajustar la amplitud del rango nos permite aplicar los thresholds y procesados de manera objetiva, sin ser afectados por las variaciones en amplitud máxima entre diferentes señales.

- **Center Clipping**: Umbral C_L=0.01 para reducir efectos de formantes
  Center clipping reduce el efecto de los formantes y limpia la autocorrelación para identificar más fácilmente el pico correspondiente al periodo de la señal. Al haber normalizado previamente la señal podemos aplicar un center clipping apropiado para toda señal de entrada.

### Métodos de estimación
- **Autocorrelación**: Método por defecto (`--method=autocorr`)
- **AMDF**: Average Magnitude Difference Function (`--method=amdf`)
  La AMDF es un método computacionalmente menos costoso a la autocorrelación pero parecido a este. Nos permite encontrar el pitch encontrando el argumento para el cual este es mínimo (fuera del origen). Tiene resultados buenos, pero como el center clipping no es tan efectivo para AMDF, los resultados con la autocorrelación son mejores.

- **Cepstrum**: Análisis cepstral (`--method=cepstrum`)
  El cepstrum también nos permite estimar el tono encontrando su valor máximo (fuera del origen). Aunque tiene la posibilidad de también dar muy buenos resultados, hemos preferido utilizar la autocorrelación, ya que el calcular el cepstrum impone una carga computacional más grande sobre el programa y su run-time es demasiado grande (~3 min para `run_get_pitch`).

### Postprocesado
- **Filtro de mediana**: Tamaño 3 (óptimo), elimina valores atípicos aislados
  Al igual que con el filtro averaging, hemos probado diferentes tamaños para el filtro de mediana para encontrar el óptimo. Encontramos los mismos problemas que con el filtro averaging, y como el caso anterior, el filtro de tamaño 3 es óptimo para evitar desplazamientos de la señal y evitar aplanar demasiado. También se probó el filtro de tamaño 7, con resultados catastróficos (<15%). Aun así, podemos observar en los resultados que el filtro de mediana de tamaño 3 acaba eliminando algunos picos y saltos que realmente existían en la señal.

- **Corrección de errores**: Elimina picos anormales (>360Hz diferencia)
  En el caso de que múltiples muestras consecutivas tengan valores erróneos, la corrección de errores nos permite ajustar el tono de salida para evitar saltos repentinos demasiado grandes para ser correctos. Se compara el tono estimado del segmento con el del segmento anterior y, si la distancia entre estos supera el threshold, se determina que existe un error y se escoge de entre las dos muestras el valor más cercano a la media de la señal, y se asigna este a ambas.

### Parámetros optimizados
  | Parámetro | Valor | Descripción |
  |-----------|-------|-------------|
  | alpha0 | -42 dB | Umbral de potencia |
  | alpha1 | 0.48 | r1/r0 normalizado |
  | alpha2 | 0.34 | rmax/r0 normalizado |
  | zc3 | 0.012*fm/2 | Tasa de cruces por cero |

  Como se ha comentado en apartados previos, se ha utilizado el script `grid_search` para optimizar los parámetros y encontrar la combinación más óptima de estos.


Evaluación *ciega* del estimador
-------------------------------

Antes de realizar el *pull request* debe asegurarse de que su repositorio contiene los ficheros necesarios
para compilar los programas correctamente ejecutando `make release`.

Con los ejecutables construidos de esta manera, los profesores de la asignatura procederán a evaluar el
estimador con la parte de test de la base de datos (desconocida para los alumnos). Una parte importante de
la nota de la práctica recaerá en el resultado de esta evaluación.
