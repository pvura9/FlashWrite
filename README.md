# *FlashWrite*: An Embedded Journaling System Backed by an Emulated EEPROM
*Designed & Implemented by Sai Vura (ECE student @ Purdue University) as a Personal Project*

## <u>Abstract</u>
The “FlashWrite” is a secure, password-protected, embedded journaling system built on an STM32 microcontroller platform. It allows users to create, store, read, view, and delete diary entries through a serial terminal interface. All data is saved to the EEPROM memory. Very simple encryption and decryption methods have been implemented for safe storage and retrieval of data into the memory of the microcontroller. Designed with limited hardware, this project emphasizes efficient memory usage, real-time input handling, and a modular firmware development. This diary serves as both a practical tool and an educational demo of embedded systems design.


## <u>Features / Demo</u>
1. System will greet users and ask them to input their password.
<br>
<img src="images/step1image.jpg" style="border: 2px solid white; border-radius: 4px;" width="250"/>
</br>
2. Users have 3 attempts to enter the correct password.
<br>
<img src="images/step2image.jpg" style="border: 2px solid white; border-radius: 4px;" width="250"/>
</br>
3. If maxed out on attempts, the system locks.
<br>
<img src="images/step3image.jpg" style="border: 2px solid white; border-radius: 4px;" width="250"/>
</br>
4. Once correct password is input, users can enter the "write," "search," "read," "list," "delete," or "logout" commands.
<br>
<img src="images/step4image.jpg" style="border: 2px solid white; border-radius: 4px;" width="250"/>
</br>
5. In case of any invalid commands, a list of valid commands will be shown.
<br>
<img src="images/step5image.jpg" style="border: 2px solid white; border-radius: 4px;" width="250"/>
</br>
6. The "write" command will enable them to make a new diary entry. They will be asked to enter a tag and the content.
<br>
<img src="images/step6image.jpg" style="border: 2px solid white; border-radius: 4px;" width="350"/>
</br>
7. Users can make multiple entries.
<br>
<img src="images/step7image.jpg" style="border: 2px solid white; border-radius: 4px;" width="350"/>
</br>
8. The "search" command will allow them to search for a certain entry based on the tagline. Details of the entry will be displayed.
<br>
<img src="images/step8image.jpg" style="border: 2px solid white; border-radius: 4px;" width="350"/>
</br>
9. If search does not match any entry, the following message will be displayed.
<br>
<img src="images/step9image.jpg" style="border: 2px solid white; border-radius: 4px;" width="350"/>
</br>
10. Users can read the specified entry with the "read" command.
<br>
<img src="images/step10image.jpg" style="border: 2px solid white; border-radius: 4px;" width="250"/>
</br>
11. Users can get a complete list of entries with the "list" command.
<br>
<img src="images/step11image.jpg" style="border: 2px solid white; border-radius: 4px;" width="250"/>
</br>
12. Users can delete a specific entry with the "delete" command.
<br>
<img src="images/step12image.jpg" style="border: 2px solid white; border-radius: 4px;" width="250"/>
</br>
13. When complete, users must enter the "logout" command to close the system.
<br>
<img src="images/step13image.jpg" style="border: 2px solid white; border-radius: 4px;" width="250"/>
</br>


## <u>System Overview</u>
- **Block Diagram of System Flow**
<br>
<img src="images/Updated Diary Block Diagram.jpg" alt="Block Diagram" width="650"/>
</br>

- **Schematic of Hardware Components**
<br>
<img src="images/hardwareSchematic.png" alt="Final Schematic" width="650"/>
</br>


## <u>System Architecture</u>
```mermaid
%%{init: {'themeVariables': { 'fontFamily': 'Times New Roman, serif' }}}%%
flowchart TB
  %% ---------- Styles ----------
  classDef userStyle fill:#d4f8d4,stroke:#333,stroke-width:1px,color:#000,rx:20,ry:20
  classDef termStyle fill:#e6d4f8,stroke:#333,stroke-width:1px,color:#000,rx:20,ry:20
  classDef mcuTextWhite color:#fff

  %% ---------- Top: User & Terminal ----------
  user[User]:::userStyle
  term[Serial Terminal]:::termStyle
  user --> term
  term -->|UART| mcu

  %% ---------- STM32 Microcontroller ----------
  subgraph mcu[STM32 Microcontroller]
    direction TB
    cp[Command Parser]:::mcuTextWhite
    dm[Diary Manager]:::mcuTextWhite
    cp --> dm

    e[Encryption + Decryption Engine]:::mcuTextWhite
    ee[EEPROM Emulator]:::mcuTextWhite
    dm -->|XOR + Key Rotation| e
    dm --> ee
  end
  style mcu fill:#d4e6f8,stroke:#333,stroke-width:1px,color:#000, rx:20, ry:20

  %% ---------- Memory ----------
  subgraph mem[Memory]
    direction TB
    fp62["Flash Page 62 (Metadata storage)"]
    fp63["Flash Page 63 (Content storage)"]
  end
  style mem fill:#fff3d4,stroke:#333,stroke-width:1px,color:#000, rx:20, ry:20

  %% ---------- Connections ----------
  ee --> mem

  %% ---------- Arrow styling (make arrows inside STM32 block black) ----------
  %% Link indices (0-based) in order of appearance:
  %% 0: user->term, 1: term->mcu, 2: cp->dm, 3: dm->e, 4: dm->ee, 5: ee->mem
  linkStyle 2 stroke:#000,stroke-width:1px,color:#000
  linkStyle 3 stroke:#000,stroke-width:1px,color:#000
  linkStyle 4 stroke:#000,stroke-width:1px,color:#000
```

## <u>Memory Management</u>
```mermaid
%%{init: {'themeVariables': { 'fontFamily': 'Times New Roman, serif' }}}%%
flowchart LR
  %% ---------- Styles ----------
  classDef box fill:#ffffff,stroke:#333,stroke-width:1px,color:#000,rx:20,ry:20
  classDef head fill:#ffffff,stroke:#333,stroke-width:1px,color:#000,rx:20,ry:20

  %% ---------- Left Column: Metadata Page ----------
  subgraph leftStack["Page 62 (Metadata)"]
    direction TB
    mh["<u>Metadata Header</u>
    tracks system version,
    entry count, next free address slot, etc."]:::head
    m1["<u>Entry #1 Metadata</u>
    tag, flashAddress, 
    length, timestamp"]:::box
    m2["<u>Entry #2 Metadata</u>
    tag, flashAddress,
    length, timestamp"]:::box
    mfree["...(free space grows downward)..."]:::box
  end
  style leftStack fill:#d4e6f8,stroke:#333,stroke-width:1px,rx:20,ry:20,color:#000

  %% ---------- Right Column: Content Page ----------
  subgraph rightStack["Page 63 (Content)"]
    direction TB
    cfree["...(free space grows upward)..."]:::box
    c2["<u>Entry #2 Content</u>
    encrypted data, length dependent on content"]:::box
    c1["<u>Entry #1 Content</u>
    encrypted data, length dependent on content"]:::box
  end
  style rightStack fill:#e6d4f8,stroke:#333,stroke-width:1px,rx:20,ry:20,color:#000

  %% ---------- Cross-links ----------
  m1 -- "flashAddress links them" --> c1
  m2 -- "flashAddress links them" --> c2
```


## <u>Implementation Details</u>
### High Level Overview of Modules
```mermaid
%%{init: { 'themeVariables': { 'fontFamily': 'Times New Roman, serif' } }}%%
flowchart TD
    %% ---------- Nodes ----------
    %% User Interface
    USER[User] --> SERIAL(("serial.c
    [CLI Parser]"))
    SERIAL --> MAIN(("main.c
    [Initialization +
    Entry Point]"))

    %% Core Logic
    MAIN --> DIARY(("diary.c
    [Entry Manager]"))
    DIARY --> CRYPTO(("crypto.c
    [Encryption]"))
    DIARY --> EEPROM(("eepromDriver.c
    Flash Storage"))

    %% Hardware Abstraction
    MAIN --> CLOCK(("clock.c
    [48MHz PLL]"))
    MAIN --> RTC(("rtc.c
    [Timekeeping]"))
    EEPROM --> FLASH[[STM32 Flash]]
    
    %% I/O Subsystems
    SERIAL --> TTY(("tty.c
    [Input Buffer]"))
    TTY --> FIFO(("fifo.c
    [Circular Queue]"))
    TTY --> SYSCALLS(("syscalls.c
    [I/O Redirection]"))
    FIFO --> SUPPORT(("support.c
    [Display Support]"))

    %% ---------- Styles ----------
    classDef tier1 fill:#ffd6cc,stroke:#333,color:#000,stroke-width:1px
    classDef tier2 fill:#ffeebc,stroke:#333,color:#000,stroke-width:1px
    classDef tier3 fill:#d6f5d6,stroke:#333,color:#000,stroke-width:1px
    classDef tier4 fill:#cce6ff,stroke:#333,color:#000,stroke-width:1px
    classDef tier5 fill:#e6ccff,stroke:#333,color:#000,stroke-width:1px

    %% Assign tiers
    class USER tier1
    class SERIAL tier2
    class MAIN,TTY tier3
    class DIARY,CLOCK,RTC,FIFO,SYSCALLS tier4
    class CRYPTO,EEPROM,FLASH,SUPPORT tier5
```

### Modules Explained
1. ```main.c```: The entry point for the application + initializes the hardware
2. ```clock.c```: Configures the internal clock system, enabling the PLL for a 48 MHz system clock
3. ```crypto.c```: Implements simple XOR encryption/decryption 
4. ```diary.c```: Manages diary entries in EEPROM, handling storage and retrieval
5. ```eepromDriver.c```: Low-level driver for the EEPROM emulation on the STM32 flash memory
6. ```fifo.c```: Implements a circular buffer for data handling and supports insertion and removal and newline detection
7. ```rtc.c```: Simulates a real-time clock using SysTick for timestamp generation in each entry
8. ```serial.c```: Handles user commands via UART I/O
9. ```syscalls.c```: Minimal system call implementations to enable standard I/O
10. ```tty.c```: Manages UART input buffering and line editing
11. ```support.c```: Provides low-level hardware and timing functions for the user interface


## <u>Bugs + Testing</u>

### Frequent Bugs Encountered:
- Flash Write Failures
- Corruption of Metadata
- FIFO Overflow
- Encryption/Decryption Mismatch
- Serial Input Issues
- Corrupted Reads After Deletion
- Timing Sensitivity

### Testing Strategies:
To ensure the reliability of the system, several testing approaches were used, covering functionality, error handling, and edge conditions. Key strategies included:
- **Unit Testing**: Verified individual modules in isolation using debug logs and tested edge cases. Wrote several helper functions to test accurate terminal output, input parsing, proper timestamping, valid data retreival, correct decryption, etc.
- **Integration Testing**: Validated interactions between each module and confirmed appropriate responses and outputs with several sessions of isolated testing.
- **Hardware Validation**: Simulated dozens of frequent writes and deletions in a short timespan to fix any timing issues and verified if RTC timestamps matched the creation times of the entries by making use of custom CLI commands and the STM32 debugger.


## <u>Tools + Datasheets</u>
The following parts were used to implement the project:
- **STM32 MCU**: https://www.technologicalarts.com/collections/student-quickbuy/products/stm32f091-dev-board 
    - Datasheets:
        - https://www.st.com/resource/en/datasheet/stm32f091cc.pdf 
        - https://www.st.com/resource/en/reference_manual/rm0091-stm32f0x1stm32f0x2stm32f0x8-advanced-armbased-32bit-mcus-stmicroelectronics.pdf  

- **ST-Link USB**: https://www.technologicalarts.com/collections/student-quickbuy/products/st-link-v2 

- **USB → UART adapter**: https://www.amazon.com/IZOKEE-CP2102-Converter-Adapter-Downloader/dp/B07D6LLX19/ref=sr_1_3?keywords=usb+to+uart+converter&qid=1691590455&sr=8-3 

- **VSCode + PlatformIO**:
    - https://code.visualstudio.com/
    - https://platformio.org/install/ide?install=vscode 
