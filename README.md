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
     void PitchAnalyzer::autocorrelation(const vector<float> &x, vector<float> &r) const {
       for (unsigned int l = 0; l < r.size(); ++l) {
         r[l] = 0;
         for (unsigned int n = l; n < x.size(); ++n){
           r[l] += x[n]*x[n-l];
         }
         r[l] = r[l]/x.size();
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
       
       if (pot < pot_threshold || zcr < 0.012*samplingFreq/2){
         return true;  // Unvoiced: too quiet or too many zero crossings
       } else if (r1norm >= r1norm_threshold && rmaxnorm >= rmaxnorm_threshold){
         return false; // Voiced: strong correlation at lag 1 and at pitch period
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

   Parámetros: pot (umbral -42 dB), r1norm (umbral 0.47), rmaxnorm (umbral 0.33), zcr

		Puede considerar, también, la conveniencia de usar la tasa de cruces por cero.

	    Recuerde configurar los paneles de datos para que el desplazamiento de ventana sea el adecuado, que
		en esta práctica es de 15 ms.

      - Use el estimador de pitch implementado en el programa `wavesurfer` en una señal de prueba y compare
	    su resultado con el obtenido por la mejor versión de su propio sistema.  Inserte una gráfica
ilustrativa del resultado de ambos estimadores.

![F0 comparison: our estimator vs Wavesurfer reference](prueba_comparison.png)
      
 		Aunque puede usar el propio Wavesurfer para obtener la representación, se valorará
	 	el uso de alternativas de mayor calidad (particularmente Python).
  
  * Optimice los parámetros de su sistema de estimación de pitch e inserte una tabla con las tasas de error
    y el *score* TOTAL proporcionados por `pitch_evaluate` en la evaluación de la base de datos 
	`pitch_db/train`..

| alpha0 (dB) | alpha1 (r1/r0) | alpha2 (rmax/r0) | TOTAL Score |
|-------------|----------------|-----------------|-------------|
| -42         | 0.47           | 0.33            | **91.42%**  |
| -40         | 0.45           | 0.35            | 91.08%      |
| -45         | 0.50           | 0.40            | 90.95%      |
| -42         | 0.50           | 0.40            | 90.87%      |
| -40         | 0.47           | 0.35            | 90.52%      |

Parámetros optimizados: alpha0=-42, alpha1=0.47, alpha2=0.33

Ejercicios de ampliación
------------------------

- Usando la librería `docopt_cpp`, modifique el fichero `get_pitch.cpp` para incorporar los parámetros del
  estimador a los argumentos de la línea de comandos.
  
  Esta técnica le resultará especialmente útil para optimizar los parámetros del estimador. Recuerde que
  una parte importante de la evaluación recaerá en el resultado obtenido en la estimación de pitch en la
  base de datos.

  * Inserte un *pantallazo* en el que se vea el mensaje de ayuda del programa y un ejemplo de utilización
    con los argumentos añadidos.

```
get_pitch - Pitch Estimator

Usage:
    get_pitch [options] <input-wav> <output-txt>
    get_pitch (-h | --help)
    get_pitch --version

Options:
    -h, --help  Show this screen
    --version   Show the version of the project
    --min-f0=<Hz>              Minimum F0 in Hz [default: 20]
    --max-f0=<Hz>              Maximum F0 in Hz [default: 500]
    --frame-len=<s>            Frame length in seconds [default: 0.030]
    --frame-shift=<s>          Frame shift in seconds [default: 0.015]
    --alpha0=<dB>              Power threshold [default: -42]
    --alpha1=<f>               r1/r0 threshold [default: 0.47]
    --alpha2=<f>               rmax/r0 threshold [default: 0.33]
    --method=<name>            Method: autocorr, amdf, cepstrum [default: autocorr]
```

Ejemplo de uso:
```bash
get_pitch --alpha0=-40 --alpha1=0.45 --alpha2=0.35 --method=cepstrum entrada.wav salida.f0
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
- **Normalización**: Ajuste de amplitud al rango [-1, 1]  
- **Center Clipping**: Umbral C_L=0.01 para reducir efectos de formantes

### Métodos de estimación
- **Autocorrelación**: Método por defecto (`--method=autocorr`)
- **AMDF**: Average Magnitude Difference Function (`--method=amdf`)
- **Cepstrum**: Análisis cepstral (`--method=cepstrum`)

### Postprocesado
- **Filtro de mediana**: Tamaño 3 (óptimo), mejora el resultado
- **Corrección de errores**: Elimina picos anormales (>360Hz diferencia)

### Parámetros optimizados
| Parámetro | Valor | Descripción |
|-----------|-------|-------------|
| alpha0 | -42 dB | Umbral de potencia |
| alpha1 | 0.47 | r1/r0 normalizado |
| alpha2 | 0.33 | rmax/r0 normalizado |
| zcr | 0.012*fm/2 | Tasa de cruces por cero |

### Análisis del filtro de mediana
- Tamaño 3: Mejora el resultado (+~1%)
- Tamaño 5: Empieza a empeorar
- Tamaño 7: Errores catastróficos (<15%)

Finestra Hamming --> No afecta pràcticament res als resultats

Filtro de mediana --> Con 3 mejora el porcentaje, con tamaño 5 empieza a empeorar, con tamaño 7 llegamos a erores catastróficos (menos de un 15% en la evaluación).

Evaluación *ciega* del estimador
-------------------------------

Antes de realizar el *pull request* debe asegurarse de que su repositorio contiene los ficheros necesarios
para compilar los programas correctamente ejecutando `make release`.

Con los ejecutables construidos de esta manera, los profesores de la asignatura procederán a evaluar el
estimador con la parte de test de la base de datos (desconocida para los alumnos). Una parte importante de
la nota de la práctica recaerá en el resultado de esta evaluación.
