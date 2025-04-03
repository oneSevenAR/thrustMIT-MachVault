
    tv_w = torch.abs(img[:, :, :, 1:] - img[:, :, :, :-1]).mean()