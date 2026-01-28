import cv2
import numpy as np
from skimage.metrics import structural_similarity as ssim
import lpips
import torch
import argparse


# ===================================================================
# MAIN FUNCTION
# ===================================================================
def compute_texture_similarity(imgA, imgB, final_size=(512, 512)):
    """
    PARAMETERS:
        imgA, imgB : numpy arrays (OpenCV images, BGR or RGB)
        final_size : resize resolution for fair comparison
    
    RETURNS:
        dictionary containing all metrics + final % similarity
    """

    # -------------------------------------------------------------------
    # PREPROCESS
    # -------------------------------------------------------------------
    imgA = cv2.resize(imgA, final_size)
    imgB = cv2.resize(imgB, final_size)

    # Convert BGR->RGB if needed
    if imgA.shape[2] == 3:
        rgbA = cv2.cvtColor(imgA, cv2.COLOR_BGR2RGB)
        rgbB = cv2.cvtColor(imgB, cv2.COLOR_BGR2RGB)
    else:
        rgbA = imgA
        rgbB = imgB

    # ===================================================================
    # HISTOGRAM SIMILARITY
    # ===================================================================
    histA = cv2.calcHist([imgA], [0,1,2], None, [32,32,32], [0,256]*3)
    histB = cv2.calcHist([imgB], [0,1,2], None, [32,32,32], [0,256]*3)

    histA = cv2.normalize(histA, None).flatten()
    histB = cv2.normalize(histB, None).flatten()

    hist_corr = cv2.compareHist(histA, histB, cv2.HISTCMP_CORREL)
    hist_corr_norm = max(0, hist_corr)

    hist_bhat = cv2.compareHist(histA, histB, cv2.HISTCMP_BHATTACHARYYA)
    hist_bhat_norm = 1.0 - min(hist_bhat, 1.0)

    hist_sim = (hist_corr_norm + hist_bhat_norm) / 2.0


    # ===================================================================
    # BRIGHTNESS & CONTRAST
    # ===================================================================
    grayA = cv2.cvtColor(imgA, cv2.COLOR_BGR2GRAY)
    grayB = cv2.cvtColor(imgB, cv2.COLOR_BGR2GRAY)

    brightness_sim = 1.0 - abs(np.mean(grayA) - np.mean(grayB)) / 255.0
    contrast_sim   = 1.0 - abs(np.std(grayA) - np.std(grayB)) / 255.0


    # ===================================================================
    # HSV COLOR SIMILARITY
    # ===================================================================
    hsvA = cv2.cvtColor(imgA, cv2.COLOR_BGR2HSV)
    hsvB = cv2.cvtColor(imgB, cv2.COLOR_BGR2HSV)

    h_diff = abs(np.mean(hsvA[:,:,0]) - np.mean(hsvB[:,:,0])) / 180.0
    s_diff = abs(np.mean(hsvA[:,:,1]) - np.mean(hsvB[:,:,1])) / 255.0
    v_diff = abs(np.mean(hsvA[:,:,2]) - np.mean(hsvB[:,:,2])) / 255.0

    hsv_sim = 1.0 - (h_diff + s_diff + v_diff) / 3.0


    # ===================================================================
    # SSIM STRUCTURAL SIMILARITY
    # ===================================================================
    ssim_score = ssim(rgbA, rgbB, channel_axis=2)
    ssim_sim = (ssim_score + 1) / 2


    # ===================================================================
    # LPIPS (AI similarity)
    # ===================================================================
    loss_fn = lpips.LPIPS(net='alex')
    tA = torch.tensor(rgbA).permute(2,0,1).unsqueeze(0).float() / 255.0
    tB = torch.tensor(rgbB).permute(2,0,1).unsqueeze(0).float() / 255.0

    lpips_dist = float(loss_fn(tA, tB))
    lpips_sim = 1.0 - min(lpips_dist, 1.0)


    # ===================================================================
    # WEIGHTED FINAL SCORE
    # ===================================================================
    metrics = np.array([
        hist_sim,
        brightness_sim,
        contrast_sim,
        hsv_sim,
        ssim_sim,
        lpips_sim
    ])

    weights = np.array([1.5, 1.0, 1.0, 1.5, 2.0, 3.0])
    weights /= np.sum(weights)

    final_similarity = float(np.sum(metrics * weights)) * 100.0

    # ===================================================================
    # RETURN RESULTS
    # ===================================================================
    return (
        f"Histogram Similarity      : {hist_sim:.4f}\n"
        f"Brightness Similarity     : {brightness_sim:.4f}\n"
        f"Contrast Similarity       : {contrast_sim:.4f}\n"
        f"HSV Similarity            : {hsv_sim:.4f}\n"
        f"SSIM Structural Similarity: {ssim_sim:.4f}\n"
        f"LPIPS AI Similarity       : {lpips_sim:.4f}\n"
        f"-----------------------------------------\n"
        f"FINAL SIMILARITY (%)      : {final_similarity:.2f}%\n"
    )

# -----------------------------------------
# Parse Command-Line Arguments
# -----------------------------------------
parser = argparse.ArgumentParser()
parser.add_argument("img1", help="First image path")
parser.add_argument("img2", help="Second image path")
args = parser.parse_args()

img1 = cv2.imread(args.img1)
img2 = cv2.imread(args.img2)

result = compute_texture_similarity(img1, img2)

print(result)