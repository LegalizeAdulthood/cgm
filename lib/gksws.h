#if !defined(GKS_WS_H)
#define GKS_WS_H

enum class Function
{
    OpenWorkstation = 2,
    CloseWorkstation = 3,
    ActivateWorkstation = 4,
    DeactivateWorkstation = 5,
    ClearWorkstation = 6,
    Polyline = 12,
    Polymarker = 13,
    Text = 14,
    FillArea = 15,
    CellArray = 16,
    SetColorRep = 48,
    SetWorkstationWindow = 54,
    SetWorkstationViewport = 55,
};

void gks_drv_cgm(Function fctid, int dx, int dy, int dimx, int *ia,
    int lr1, double *r1, int lr2, double *r2, int lc, char *chars,
    void **context);

#endif
