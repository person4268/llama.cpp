import argparse
import torch
from transformers import AutoModel, AutoTokenizer

ap = argparse.ArgumentParser()
ap.add_argument("-m", "--model", help="Path to InternVL2 model")
args = ap.parse_args()

# find the model part that includes the the multimodal projector weights
model = AutoModel.from_pretrained(args.model, trust_remote_code=True, local_files_only=True)
checkpoint = model.state_dict()

# get a list of mlp tensor names
mlp_tensors = [k for k, v in checkpoint.items() if k.startswith("mlp")]

# store these tensors in a new dictionary and torch.save them
mlp = {name: checkpoint[name].float() for name in mlp_tensors}
torch.save(mlp, f"{args.model}/internvl2.mlp")

config = model.language_model.config

model.language_model.save_pretrained(f"{args.model}/model")
tok = AutoTokenizer.from_pretrained(args.model, trust_remote_code=True)
tok.save_pretrained(f"{args.model}/model")

print("Done!")
print(f"Now you can convert {args.model} to a regular LLaMA GGUF file.")
print(f"Also, use {args.model}/internvl2.mlp and the original model to prepare a internvit-plus-mlp.gguf file.")
