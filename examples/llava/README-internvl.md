```bash
# this extracts the language model part and gives you the MLP extracted into a second filer (tbh you don't really need to i should remove that)
python internvl-surgery.py -m /mnt4/hf/hub/models--OpenGVLab--InternVL2-1B/snapshots/000f0c7031f031007376d81e44417cc507d2f08f
# i copied the output to /mnt4/models/internvl1b-surgery/ afterwards fyi
# this should give you an mmproj file
rm /mnt4/models/internvl1b-surgery/mmproj-model-f16.gguf; python convert_image_encoder_to_gguf.py -m /mnt4/hf/hub/models--OpenGVLab--InternVL2-1B/snapshots/000f0c7031f031007376d81e44417cc507d2f08f --llava-projector /mnt4/models/internvl1b-surgery/internvl2.mlp --internvl -o /mnt4/models/internvl1b-surgery/
../../build/bin/llama-llava-cli -m /mnt4/models/internvl1b-surgery/Qwen2-0.5B-Instruct-F16.gguf --mmproj /mnt4/models/internvl1b-surgery/mmproj-model-f16.gguf --image ~/Downloads/1711177507080914.jpg -ngl 18 -i -p "<image> Describe the image in detail."
```