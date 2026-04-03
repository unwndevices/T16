// DO NOT EDIT — generated from schema/t16-config.schema.json

export interface T16Configuration {
  version: 200;
  global: {
    mode: number;
    sensitivity: number;
    brightness: number;
    midi_trs: number;
    trs_type: number;
    passthrough: number;
    midi_ble: number;
    /**
     * @minItems 16
     * @maxItems 16
     */
    custom_scale1: [
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number
    ];
    /**
     * @minItems 16
     * @maxItems 16
     */
    custom_scale2: [
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number,
      number
    ];
  };
  /**
   * @minItems 4
   * @maxItems 4
   */
  banks: [
    {
      ch: number;
      scale: number;
      oct: number;
      note: number;
      vel: number;
      at: number;
      flip_x: number;
      flip_y: number;
      koala_mode: number;
      /**
       * @minItems 8
       * @maxItems 8
       */
      chs: [number, number, number, number, number, number, number, number];
      /**
       * @minItems 8
       * @maxItems 8
       */
      ids: [number, number, number, number, number, number, number, number];
    },
    {
      ch: number;
      scale: number;
      oct: number;
      note: number;
      vel: number;
      at: number;
      flip_x: number;
      flip_y: number;
      koala_mode: number;
      /**
       * @minItems 8
       * @maxItems 8
       */
      chs: [number, number, number, number, number, number, number, number];
      /**
       * @minItems 8
       * @maxItems 8
       */
      ids: [number, number, number, number, number, number, number, number];
    },
    {
      ch: number;
      scale: number;
      oct: number;
      note: number;
      vel: number;
      at: number;
      flip_x: number;
      flip_y: number;
      koala_mode: number;
      /**
       * @minItems 8
       * @maxItems 8
       */
      chs: [number, number, number, number, number, number, number, number];
      /**
       * @minItems 8
       * @maxItems 8
       */
      ids: [number, number, number, number, number, number, number, number];
    },
    {
      ch: number;
      scale: number;
      oct: number;
      note: number;
      vel: number;
      at: number;
      flip_x: number;
      flip_y: number;
      koala_mode: number;
      /**
       * @minItems 8
       * @maxItems 8
       */
      chs: [number, number, number, number, number, number, number, number];
      /**
       * @minItems 8
       * @maxItems 8
       */
      ids: [number, number, number, number, number, number, number, number];
    }
  ];
}
