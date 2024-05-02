# Human-Detection-System-for-Mars-Habitat
Masters thesis at University of Bremen.\
Abstract \
n the context of space exploration, advancements have usually been the result of a series of gradual research initiatives. The next phase in this research is Mars mission, to send humans on Mars.
ZARM - Center of Applied Space Technology and Microgravity, in collaboration with the University of Bremen, is working on “Human on Mars Initiative”. Drawing inspiration from this initiative, the proposed thesis work involves the creation of a surveillance system designed to detect the
presence of humans in a given environment. Tailored for terrestrial applications, this system must
contend with constraints such as limited hardware resources, power consumption, and weight limitations. Addressing these challenges, this work focuses on implementing machine learning models
on compact, resource constrained devices, such as microcontrollers, aligning seamlessly with the
research requirements. A significant aspect of work involves challenging the limitations posed by
conventional image processing detection models. These models often impose heavy demands on
memory and processing capabilities, making them impractical for resource-limited environments.
Instead, the thesis advocates for the adoption of classification models for object detection, which
provide a more lightweight alternative. To operationalize these concepts, a low-end microcontroller device has been meticulously chosen for model implementation. Ultimately, the aims is
to achieve real-time detection capabilities, enabling the system to promptly transmit event-based
updates upon detecting human presence in a frame. Leveraging the LoRaWAN network, these
updates can be transferred seamlessly within seconds, facilitating efficient surveillance in various
environments
This repository contains the execution files for this thesis work.\
The file is structured as
1. platformio - On device inference file, with all the depended libraries
2. edge_impulse - link to the the developed edge impulse project and zip file for all dataset for test cases
3. offline_inference - code to infer offline keras model and to generate pr curve for ondevice and keras inference



