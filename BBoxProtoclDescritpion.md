# BBox Protocol for Telescope Control

The BBox protocol is a communication standard used by telescope control systems and digital setting circles (DSCs) to interface with astronomy software for precise control and tracking of celestial objects. It operates over a serial connection (often RS-232) and allows the software to receive and interpret data from encoders mounted on the telescope's axes. The protocol includes commands for querying and configuring the telescope's position, encoder settings, and other operational parameters.

## Key Commands in the BBox Protocol

### 1. Q Command (Query Encoder Position)
- **Purpose**: Retrieves the current encoder counts for the azimuth (horizontal) and altitude (vertical) axes, representing the telescope's pointing position.
- **Command Format**:

Q

- **Response Format**:

azimuth_count, altitude_count

- **Example Interaction**:
- **Sent Command**: `Q`
- **Response**: `123456,654321`
- Here, `123456` is the encoder count for the azimuth axis, and `654321` is the encoder count for the altitude axis. These counts are used by the software to determine the telescope's exact pointing coordinates.

### 2. H Command (Set Home Position)
- **Purpose**: Resets or defines a "home" or reference position for the telescope’s encoders.
- **Command Format**:

H

- **Response Format**:

OK

- **Example Interaction**:
- **Sent Command**: `H`
- **Response**: `OK`
- `OK` confirms that the home position has been set successfully. This position can be used as a baseline for alignment or recalibration purposes.

### 3. G Command (Get Encoder Resolution)
- **Purpose**: Retrieves the encoder resolution, which represents the number of steps per full revolution for each axis.
- **Command Format**:

G

- **Response Format**:

azimuth_resolution, altitude_resolution

- **Example Interaction**:
- **Sent Command**: `G`
- **Response**: `4096,8192`
- Here, `4096` is the azimuth resolution, and `8192` is the altitude resolution, representing the number of encoder steps per revolution for each axis.

### 4. Z Command (Reset Encoders)
- **Purpose**: Resets the encoder counts to zero or a predefined starting position.
- **Command Format**:

Z

- **Response Format**:

OK

- **Example Interaction**:
- **Sent Command**: `Z`
- **Response**: `OK`
- This response confirms that the encoder counts have been reset successfully. After resetting, the telescope is ready for further alignment or movement instructions.

### 5. S Command (Set Encoder Resolution)
- **Purpose**: Configures the encoder resolution, specifying the number of steps per full revolution for both azimuth and altitude axes.
- **Command Format**:

S azimuth_resolution, altitude_resolution

- **Response Format**:

OK

- **Example Interaction**:
- **Sent Command**: `S 4096,8192`
- **Response**: `OK`
- This confirms that the encoder resolution has been set to 4096 steps per revolution for the azimuth axis and 8192 steps per revolution for the altitude axis.

### 6. T Command (Toggle Tracking)
- **Purpose**: Enables or disables tracking to keep the telescope aligned with a specific celestial object.
- **Command Format**:

T on/off

- **Response Format**:

OK

- **Example Interaction**:
- **Sent Command**: `T on`
- **Response**: `OK`
- This indicates that tracking has been enabled. Similarly, sending `T off` would disable tracking.

### 7. C Command (Set Calibration)
- **Purpose**: Applies a calibration factor or offset to correct for mechanical inaccuracies in the telescope mount or encoder setup.
- **Command Format**:

C calibration_value

- **Response Format**:

OK

- **Example Interaction**:
- **Sent Command**: `C 5`
- **Response**: `OK`
- This confirms that a calibration factor of 5 has been applied.

### 8. E Command (Error Query)
- **Purpose**: Retrieves any error codes or status flags from the digital setting circle, assisting with diagnostics or troubleshooting.
- **Command Format**:

E

- **Response Format**:

error_code

- **Example Interaction**:
- **Sent Command**: `E`
- **Response**: `0`
- A response of `0` typically means no error is present. Non-zero values would indicate specific error codes, which may require consulting device documentation for interpretation.

## Summary of Common Command Responses

| Command | Description                  | Example Sent | Example Response      |
|---------|------------------------------|--------------|-----------------------|
| `Q`     | Query Encoder Position       | `Q`          | `123456,654321`       |
| `H`     | Set Home Position            | `H`          | `OK`                  |
| `G`     | Get Encoder Resolution       | `G`          | `4096,8192`           |
| `Z`     | Reset Encoders               | `Z`          | `OK`                  |
| `S`     | Set Encoder Resolution       | `S 4096,8192`| `OK`                  |
| `T`     | Toggle Tracking              | `T on`       | `OK`                  |
| `C`     | Set Calibration              | `C 5`        | `OK`                  |
| `E`     | Error Query                  | `E`          | `0`                   |

Each command in the BBox protocol plays a crucial role in aligning, configuring, and operating the telescope. By understanding the function and response format of each command, users can accurately control and adjust the telescope’s movement, alignment, and tracking capabilities with compatible software applications.