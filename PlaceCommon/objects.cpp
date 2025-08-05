#include <objects.h>
#include "placedb.h"

void Net::addPin(Pin *pin)
{
    netPins.push_back(pin);
}

int Net::getPinCount()
{
    return netPins.size();
}

void Net::allocateMemoryForPin(int n)
{
    netPins.reserve(n);
}

double Net::calcNetHPWL()
{
    double maxX = -DOUBLE_MAX;
    double minX = DOUBLE_MAX;
    // double maxY = DOUBLE_MIN;
    double maxY = -DOUBLE_MAX;
    double minY = DOUBLE_MAX;
    // double maxZ = DOUBLE_MIN;
    double maxZ = -DOUBLE_MAX; // potential bug: double_min >0 so boundPinZmax might be null when all z == 0
    double minZ = DOUBLE_MAX;

    double curX;
    double curY;
    double curZ;
    POS_3D curPos;
    double HPWL;
    for (Pin *curPin : netPins)
    {
        // curPos = curPin->getAbsolutePos();
        curPos=curPin->absolutePos;
        curX = curPos.x;
        curY = curPos.y;
        curZ = curPos.z;
        minX = min(minX, curX);
        maxX = max(maxX, curX);
        minY = min(minY, curY);
        maxY = max(maxY, curY);
        minZ = min(minZ, curZ);
        maxZ = max(maxZ, curZ);
    }
    if (!gArg.CheckExist("3DIC"))
    {
        //? assert(maxZ == minZ == 0); this causes bug
        assert(float_equal(maxZ, 0.0));
        assert(float_equal(minZ, 0.0));
    }
    HPWL = ((maxX - minX) + (maxY - minY) + (maxZ - minZ));
    return HPWL;
}

double Net::calcBoundPin()
{
    // double maxX = DOUBLE_MIN;
    double maxX = -DOUBLE_MAX;
    double minX = DOUBLE_MAX;
    // double maxY = DOUBLE_MIN;
    double maxY = -DOUBLE_MAX;
    double minY = DOUBLE_MAX;
    // double maxZ = DOUBLE_MIN;
    double maxZ = -DOUBLE_MAX; // potential bug: double_min >0 so boundPinZmax might be null when all z == 0
    double minZ = DOUBLE_MAX;

    double curX;
    double curY;
    double curZ;
    POS_3D curPos;
    double HPWL;

    for (Pin *curPin : netPins)
    {
        curPos = curPin->getAbsolutePos();
        curX = curPos.x;
        curY = curPos.y;
        curZ = curPos.z;
        //!!!!! assume curX curY curZ always >= 0!!!
        assert(curZ == 0);
        if (curX < minX)
        {
            minX = curX;
            boundPinXmin = curPin;
        }

        if (curX > maxX)
        {
            maxX = curX;
            boundPinXmax = curPin;
        }

        if (curY < minY)
        {
            minY = curY;
            boundPinYmin = curPin;
        }

        if (curY > maxY)
        {
            maxY = curY;
            boundPinYmax = curPin;
        }

        if (curZ < minZ)
        {
            minZ = curZ;
            boundPinZmin = curPin;
        }

        if (curZ > maxZ)
        {
            maxZ = curZ;
            boundPinZmax = curPin;
        }
    }
    if (!gArg.CheckExist("3DIC"))
    {
        assert(float_equal(maxZ, 0.0));
        assert(float_equal(minZ, 0.0));
    }
    HPWL = ((maxX - minX) + (maxY - minY) + (maxZ - minZ));
    return HPWL;
}

void Net::clearBoundPins()
{
    boundPinXmax = NULL;
    boundPinXmin = NULL;
    boundPinYmax = NULL;
    boundPinYmin = NULL;
    boundPinZmax = NULL;
    boundPinZmin = NULL;
}

double Net::calcBoundPin_3D()
{
    double maxX = -DOUBLE_MAX;
    double minX = DOUBLE_MAX;
    double maxY = -DOUBLE_MAX;
    double minY = DOUBLE_MAX;
    double maxZ = -DOUBLE_MAX;
    double minZ = DOUBLE_MAX;

    double curX;
    double curY;
    double curZ;
    POS_3D curPos;
    double HPWL;

    for (Pin *curPin : netPins)
    {
        curPos = curPin->getAbsolutePos();
        curX = curPos.x;
        curY = curPos.y;
        curZ = curPos.z;
        
        // No assertion for Z=0 in true 3D version
        if (curX < minX)
        {
            minX = curX;
            boundPinXmin = curPin;
        }

        if (curX > maxX)
        {
            maxX = curX;
            boundPinXmax = curPin;
        }

        if (curY < minY)
        {
            minY = curY;
            boundPinYmin = curPin;
        }

        if (curY > maxY)
        {
            maxY = curY;
            boundPinYmax = curPin;
        }

        if (curZ < minZ)
        {
            minZ = curZ;
            boundPinZmin = curPin;
        }

        if (curZ > maxZ)
        {
            maxZ = curZ;
            boundPinZmax = curPin;
        }
    }
    
    HPWL = ((maxX - minX) + (maxY - minY) + (maxZ - minZ));
    return HPWL;
}

double Net::calcWirelengthLSE_2D(VECTOR_2D invertedGamma)
{
    VECTOR_2D sumMax;
    VECTOR_2D sumMin;
    sumMax.SetZero();
    sumMin.SetZero();

    assert(boundPinXmax);
    assert(boundPinXmin);
    assert(boundPinYmax);
    assert(boundPinYmin);

    float pinMaxX = boundPinXmax->getAbsolutePos().x;
    float pinMaxY = boundPinYmax->getAbsolutePos().y;

    float pinMinX = boundPinXmin->getAbsolutePos().x;
    float pinMinY = boundPinYmin->getAbsolutePos().y;

    for (Pin *curPin : netPins)
    {
        POS_3D pinPosition = curPin->getAbsolutePos();
        VECTOR_2D expMax;
        VECTOR_2D expMin;
        expMax.x = (pinPosition.x - pinMaxX) * invertedGamma.x;
        expMin.x = (pinMinX - pinPosition.x) * invertedGamma.x;
        expMax.y = (pinPosition.y - pinMaxY) * invertedGamma.y;
        expMin.y = (pinMinY - pinPosition.y) * invertedGamma.y;

        if (expMax.x > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_LSE.x = fastExp(expMax.x);
            sumMax.x += curPin->eMax_LSE.x;
            curPin->expZeroFlgMax_LSE.x = false;
        }
        else
        {
            curPin->expZeroFlgMax_LSE.x = true;
        }

        if (expMin.x > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_LSE.x = fastExp(expMin.x);
            sumMin.x += curPin->eMin_LSE.x;
            curPin->expZeroFlgMin_LSE.x = false;
        }
        else
        {
            curPin->expZeroFlgMin_LSE.x = true;
        }

        if (expMax.y > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_LSE.y = fastExp(expMax.y);
            sumMax.y += curPin->eMax_LSE.y;
            curPin->expZeroFlgMax_LSE.y = false;
        }
        else
        {
            curPin->expZeroFlgMax_LSE.y = true;
        }

        if (expMin.y > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_LSE.y = fastExp(expMin.y);
            sumMin.y += curPin->eMin_LSE.y;
            curPin->expZeroFlgMin_LSE.y = false;
        }
        else
        {
            curPin->expZeroFlgMin_LSE.y = true;
        }
    }

    sumMax_LSE.x = sumMax.x;
    sumMax_LSE.y = sumMax.y;
    sumMin_LSE.x = sumMin.x;
    sumMin_LSE.y = sumMin.y;

    return (pinMaxX - pinMinX + log(sumMax.x) / invertedGamma.x + log(sumMin.x) / invertedGamma.x) +
           (pinMaxY - pinMinY + log(sumMax.y) / invertedGamma.y + log(sumMin.y) / invertedGamma.y);
}

double Net::calcWirelengthWA_2D(VECTOR_2D invertedGamma)
{
    VECTOR_2D numeratorMax;
    VECTOR_2D denominatorMax;
    VECTOR_2D numeratorMin;
    VECTOR_2D denominatorMin;

    //! WA wirelength model, see NTUPlace 3D paper page 6 : Stable Weighted-Average Wirelength Model
    //! Here on X/Y dimension: WA wirelength = numeratorMax/denominatorMax - numeratorMin/denominatorMin, total wirelength = wirelength on X dimension + wirelength on Y dimension
    //! numerator and denominator are sum of the results of all pins, see the code below

    numeratorMax.SetZero();
    denominatorMax.SetZero();
    numeratorMin.SetZero();
    denominatorMin.SetZero();

    assert(boundPinXmax);
    assert(boundPinXmin);
    assert(boundPinYmax);
    assert(boundPinYmin);

    float pinMaxX = boundPinXmax->getAbsolutePos().x;
    float pinMaxY = boundPinYmax->getAbsolutePos().y;

    float pinMinX = boundPinXmin->getAbsolutePos().x;
    float pinMinY = boundPinYmin->getAbsolutePos().y;

    for (Pin *curPin : netPins)
    {
        POS_3D pinPosition = curPin->getAbsolutePos();
        VECTOR_2D expMax;                                       // (Xi-Xmax)/gamma in WA model (X/Y/Z)
        VECTOR_2D expMin;                                       // (Xmin-Xi)/gamma in WA model (X/Y/Z)
        expMax.x = (pinPosition.x - pinMaxX) * invertedGamma.x; //! wlen_cof is actually 1/gamma
        expMin.x = (pinMinX - pinPosition.x) * invertedGamma.x; //! wlen_cof used here!
        expMax.y = (pinPosition.y - pinMaxY) * invertedGamma.y;
        expMin.y = (pinMinY - pinPosition.y) * invertedGamma.y;
        // cout<<padding<<"expmax: "<<exp_max_x<<endl;
        if (expMax.x > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_WA.x = fastExp(expMax.x);
            numeratorMax.x += pinPosition.x * curPin->eMax_WA.x;
            denominatorMax.x += curPin->eMax_WA.x;
            curPin->expZeroFlgMax_WA.x = false;
        }
        else
        {
            curPin->expZeroFlgMax_WA.x = true;
        }

        if (expMin.x > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_WA.x = fastExp(expMin.x);
            numeratorMin.x += pinPosition.x * curPin->eMin_WA.x;
            denominatorMin.x += curPin->eMin_WA.x;
            curPin->expZeroFlgMin_WA.x = false;
        }
        else
        {
            curPin->expZeroFlgMin_WA.x = true;
        }

        if (expMax.y > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_WA.y = fastExp(expMax.y);
            numeratorMax.y += pinPosition.y * curPin->eMax_WA.y;
            denominatorMax.y += curPin->eMax_WA.y;
            curPin->expZeroFlgMax_WA.y = false;
        }
        else
        {
            curPin->expZeroFlgMax_WA.y = true;
        }

        if (expMin.y > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_WA.y = fastExp(expMin.y);
            numeratorMin.y += pinPosition.y * curPin->eMin_WA.y;
            denominatorMin.y += curPin->eMin_WA.y;
            curPin->expZeroFlgMin_WA.y = false;
        }
        else
        {
            curPin->expZeroFlgMin_WA.y = true;
        }
    }

    numeratorMax_WA.x = numeratorMax.x;
    numeratorMax_WA.y = numeratorMax.y;
    denominatorMax_WA.x = denominatorMax.x;
    denominatorMax_WA.y = denominatorMax.y;

    numeratorMin_WA.x = numeratorMin.x;
    numeratorMin_WA.y = numeratorMin.y;
    denominatorMin_WA.x = denominatorMin.x;
    denominatorMin_WA.y = denominatorMin.y;

    return (numeratorMax_WA.x / denominatorMax_WA.x - numeratorMin_WA.x / denominatorMin_WA.x) + (numeratorMax_WA.y / denominatorMax_WA.y - numeratorMin_WA.y / denominatorMin_WA.y);
}

VECTOR_2D Net::getWirelengthGradientWA_2D(VECTOR_2D invertedGamma, Pin *curPin)
{
    assert(curPin);
    VECTOR_2D gradientOnCurrentPin = VECTOR_2D();
    VECTOR_2D gradientNumeratorMax = VECTOR_2D();
    VECTOR_2D gradientDenominatorMax = VECTOR_2D();
    VECTOR_2D gradientNumeratorMin = VECTOR_2D();
    VECTOR_2D gradientDenominatorMin = VECTOR_2D();
    VECTOR_2D gradientMax = VECTOR_2D();
    VECTOR_2D gradientMin = VECTOR_2D();
    // ? no SetZero here (called in default constructor)
    //? assert(gradientOnCurrentPin.x == gradientDenominatorMin.y == 0.0);
    assert(gradientOnCurrentPin.x == 0.0);
    assert(gradientDenominatorMin.y == 0.0);

    POS_3D curPinPosition = curPin->getAbsolutePos();

    if (!curPin->expZeroFlgMax_WA.x)
    { // if flg=0, assume grad=0
        gradientDenominatorMax.x = invertedGamma.x * curPin->eMax_WA.x;
        gradientNumeratorMax.x = curPin->eMax_WA.x + curPinPosition.x * gradientDenominatorMax.x;
        gradientMax.x =
            (gradientNumeratorMax.x * denominatorMax_WA.x - gradientDenominatorMax.x * numeratorMax_WA.x) /
            (denominatorMax_WA.x * denominatorMax_WA.x);
    }

    if (!curPin->expZeroFlgMax_WA.y)
    {
        gradientDenominatorMax.y = invertedGamma.y * curPin->eMax_WA.y;
        gradientNumeratorMax.y = curPin->eMax_WA.y + curPinPosition.y * gradientDenominatorMax.y;
        gradientMax.y =
            (gradientNumeratorMax.y * denominatorMax_WA.y - gradientDenominatorMax.y * numeratorMax_WA.y) /
            (denominatorMax_WA.y * denominatorMax_WA.y);
    }

    if (!curPin->expZeroFlgMin_WA.x)
    {
        gradientDenominatorMin.x = invertedGamma.x * curPin->eMin_WA.x;
        gradientNumeratorMin.x = curPin->eMin_WA.x - curPinPosition.x * gradientDenominatorMin.x;
        gradientMin.x =
            (gradientNumeratorMin.x * denominatorMin_WA.x + gradientDenominatorMin.x * numeratorMin_WA.x) /
            (denominatorMin_WA.x * denominatorMin_WA.x);
    }

    if (!curPin->expZeroFlgMin_WA.y)
    {
        gradientDenominatorMin.y = invertedGamma.y * curPin->eMin_WA.y;
        gradientNumeratorMin.y = curPin->eMin_WA.y - curPinPosition.y * gradientDenominatorMin.y;
        gradientMin.y =
            (gradientNumeratorMin.y * denominatorMin_WA.y + gradientDenominatorMin.y * numeratorMin_WA.y) /
            (denominatorMin_WA.y * denominatorMin_WA.y);
    }

    gradientOnCurrentPin.x = gradientMax.x - gradientMin.x;
    gradientOnCurrentPin.y = gradientMax.y - gradientMin.y;
    return gradientOnCurrentPin;
}

VECTOR_2D Net::getWirelengthGradientLSE_2D(VECTOR_2D invertedGamma, Pin *curPin)
{
    VECTOR_2D gradientOnCurrentPin = VECTOR_2D();
    VECTOR_2D gradientMax = VECTOR_2D(); // the gradient added by positive term
    VECTOR_2D gradientMin = VECTOR_2D(); // the gradient added by negative term

    gradientMax.x = (curPin->expZeroFlgMax_LSE.x ? 0 : curPin->eMax_LSE.x) / sumMax_LSE.x;
    gradientMin.x = (curPin->expZeroFlgMin_LSE.x ? 0 : curPin->eMin_LSE.x) / sumMin_LSE.x;
    gradientMax.y = (curPin->expZeroFlgMax_LSE.y ? 0 : curPin->eMax_LSE.y) / sumMax_LSE.y;
    gradientMin.y = (curPin->expZeroFlgMin_LSE.y ? 0 : curPin->eMin_LSE.y) / sumMin_LSE.y;

    gradientOnCurrentPin.x = gradientMax.x - gradientMin.x;
    gradientOnCurrentPin.y = gradientMax.y - gradientMin.y;
    return gradientOnCurrentPin;
}

//========================================================================
// 3D Wirelength Models Implementation
//========================================================================

double Net::calcWirelengthLSE_3D(VECTOR_3D invertedGamma)
{
    VECTOR_3D sumMax;
    VECTOR_3D sumMin;
    sumMax.SetZero();
    sumMin.SetZero();

    assert(boundPinXmax);
    assert(boundPinXmin);
    assert(boundPinYmax);
    assert(boundPinYmin);
    assert(boundPinZmax);
    assert(boundPinZmin);

    float pinMaxX = boundPinXmax->getAbsolutePos().x;
    float pinMaxY = boundPinYmax->getAbsolutePos().y;
    float pinMaxZ = boundPinZmax->getAbsolutePos().z;

    float pinMinX = boundPinXmin->getAbsolutePos().x;
    float pinMinY = boundPinYmin->getAbsolutePos().y;
    float pinMinZ = boundPinZmin->getAbsolutePos().z;

    for (Pin *curPin : netPins)
    {
        POS_3D pinPosition = curPin->getAbsolutePos();
        VECTOR_3D expMax;
        VECTOR_3D expMin;
        expMax.x = (pinPosition.x - pinMaxX) * invertedGamma.x;
        expMin.x = (pinMinX - pinPosition.x) * invertedGamma.x;
        expMax.y = (pinPosition.y - pinMaxY) * invertedGamma.y;
        expMin.y = (pinMinY - pinPosition.y) * invertedGamma.y;
        expMax.z = (pinPosition.z - pinMaxZ) * invertedGamma.z;
        expMin.z = (pinMinZ - pinPosition.z) * invertedGamma.z;

        // X dimension
        if (expMax.x > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_LSE.x = fastExp(expMax.x);
            sumMax.x += curPin->eMax_LSE.x;
            curPin->expZeroFlgMax_LSE.x = false;
        }
        else
        {
            curPin->expZeroFlgMax_LSE.x = true;
        }

        if (expMin.x > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_LSE.x = fastExp(expMin.x);
            sumMin.x += curPin->eMin_LSE.x;
            curPin->expZeroFlgMin_LSE.x = false;
        }
        else
        {
            curPin->expZeroFlgMin_LSE.x = true;
        }

        // Y dimension
        if (expMax.y > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_LSE.y = fastExp(expMax.y);
            sumMax.y += curPin->eMax_LSE.y;
            curPin->expZeroFlgMax_LSE.y = false;
        }
        else
        {
            curPin->expZeroFlgMax_LSE.y = true;
        }

        if (expMin.y > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_LSE.y = fastExp(expMin.y);
            sumMin.y += curPin->eMin_LSE.y;
            curPin->expZeroFlgMin_LSE.y = false;
        }
        else
        {
            curPin->expZeroFlgMin_LSE.y = true;
        }

        // Z dimension
        if (expMax.z > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_LSE.z = fastExp(expMax.z);
            sumMax.z += curPin->eMax_LSE.z;
            curPin->expZeroFlgMax_LSE.z = false;
        }
        else
        {
            curPin->expZeroFlgMax_LSE.z = true;
        }

        if (expMin.z > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_LSE.z = fastExp(expMin.z);
            sumMin.z += curPin->eMin_LSE.z;
            curPin->expZeroFlgMin_LSE.z = false;
        }
        else
        {
            curPin->expZeroFlgMin_LSE.z = true;
        }
    }

    sumMax_LSE.x = sumMax.x;
    sumMax_LSE.y = sumMax.y;
    sumMax_LSE.z = sumMax.z;
    sumMin_LSE.x = sumMin.x;
    sumMin_LSE.y = sumMin.y;
    sumMin_LSE.z = sumMin.z;

    return (pinMaxX - pinMinX + log(sumMax.x) / invertedGamma.x + log(sumMin.x) / invertedGamma.x) +
           (pinMaxY - pinMinY + log(sumMax.y) / invertedGamma.y + log(sumMin.y) / invertedGamma.y) +
           (pinMaxZ - pinMinZ + log(sumMax.z) / invertedGamma.z + log(sumMin.z) / invertedGamma.z);
}

double Net::calcWirelengthWA_3D(VECTOR_3D invertedGamma)
{
    VECTOR_3D numeratorMax;
    VECTOR_3D denominatorMax;
    VECTOR_3D numeratorMin;
    VECTOR_3D denominatorMin;

    numeratorMax.SetZero();
    denominatorMax.SetZero();
    numeratorMin.SetZero();
    denominatorMin.SetZero();

    assert(boundPinXmax);
    assert(boundPinXmin);
    assert(boundPinYmax);
    assert(boundPinYmin);
    assert(boundPinZmax);
    assert(boundPinZmin);

    float pinMaxX = boundPinXmax->getAbsolutePos().x;
    float pinMaxY = boundPinYmax->getAbsolutePos().y;
    float pinMaxZ = boundPinZmax->getAbsolutePos().z;

    float pinMinX = boundPinXmin->getAbsolutePos().x;
    float pinMinY = boundPinYmin->getAbsolutePos().y;
    float pinMinZ = boundPinZmin->getAbsolutePos().z;

    for (Pin *curPin : netPins)
    {
        POS_3D pinPosition = curPin->getAbsolutePos();
        VECTOR_3D expMax;
        VECTOR_3D expMin;
        expMax.x = (pinPosition.x - pinMaxX) * invertedGamma.x;
        expMin.x = (pinMinX - pinPosition.x) * invertedGamma.x;
        expMax.y = (pinPosition.y - pinMaxY) * invertedGamma.y;
        expMin.y = (pinMinY - pinPosition.y) * invertedGamma.y;
        expMax.z = (pinPosition.z - pinMaxZ) * invertedGamma.z;
        expMin.z = (pinMinZ - pinPosition.z) * invertedGamma.z;

        // X dimension
        if (expMax.x > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_WA.x = fastExp(expMax.x);
            numeratorMax.x += pinPosition.x * curPin->eMax_WA.x;
            denominatorMax.x += curPin->eMax_WA.x;
            curPin->expZeroFlgMax_WA.x = false;
        }
        else
        {
            curPin->expZeroFlgMax_WA.x = true;
        }

        if (expMin.x > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_WA.x = fastExp(expMin.x);
            numeratorMin.x += pinPosition.x * curPin->eMin_WA.x;
            denominatorMin.x += curPin->eMin_WA.x;
            curPin->expZeroFlgMin_WA.x = false;
        }
        else
        {
            curPin->expZeroFlgMin_WA.x = true;
        }

        // Y dimension
        if (expMax.y > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_WA.y = fastExp(expMax.y);
            numeratorMax.y += pinPosition.y * curPin->eMax_WA.y;
            denominatorMax.y += curPin->eMax_WA.y;
            curPin->expZeroFlgMax_WA.y = false;
        }
        else
        {
            curPin->expZeroFlgMax_WA.y = true;
        }

        if (expMin.y > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_WA.y = fastExp(expMin.y);
            numeratorMin.y += pinPosition.y * curPin->eMin_WA.y;
            denominatorMin.y += curPin->eMin_WA.y;
            curPin->expZeroFlgMin_WA.y = false;
        }
        else
        {
            curPin->expZeroFlgMin_WA.y = true;
        }

        // Z dimension
        if (expMax.z > NEGATIVE_MAX_EXP)
        {
            curPin->eMax_WA.z = fastExp(expMax.z);
            numeratorMax.z += pinPosition.z * curPin->eMax_WA.z;
            denominatorMax.z += curPin->eMax_WA.z;
            curPin->expZeroFlgMax_WA.z = false;
        }
        else
        {
            curPin->expZeroFlgMax_WA.z = true;
        }

        if (expMin.z > NEGATIVE_MAX_EXP)
        {
            curPin->eMin_WA.z = fastExp(expMin.z);
            numeratorMin.z += pinPosition.z * curPin->eMin_WA.z;
            denominatorMin.z += curPin->eMin_WA.z;
            curPin->expZeroFlgMin_WA.z = false;
        }
        else
        {
            curPin->expZeroFlgMin_WA.z = true;
        }
    }

    numeratorMax_WA.x = numeratorMax.x;
    numeratorMax_WA.y = numeratorMax.y;
    numeratorMax_WA.z = numeratorMax.z;
    denominatorMax_WA.x = denominatorMax.x;
    denominatorMax_WA.y = denominatorMax.y;
    denominatorMax_WA.z = denominatorMax.z;

    numeratorMin_WA.x = numeratorMin.x;
    numeratorMin_WA.y = numeratorMin.y;
    numeratorMin_WA.z = numeratorMin.z;
    denominatorMin_WA.x = denominatorMin.x;
    denominatorMin_WA.y = denominatorMin.y;
    denominatorMin_WA.z = denominatorMin.z;

    return (numeratorMax_WA.x / denominatorMax_WA.x - numeratorMin_WA.x / denominatorMin_WA.x) + 
           (numeratorMax_WA.y / denominatorMax_WA.y - numeratorMin_WA.y / denominatorMin_WA.y) +
           (numeratorMax_WA.z / denominatorMax_WA.z - numeratorMin_WA.z / denominatorMin_WA.z);
}

VECTOR_3D Net::getWirelengthGradientWA_3D(VECTOR_3D invertedGamma, Pin *curPin)
{
    assert(curPin);
    VECTOR_3D gradientOnCurrentPin = VECTOR_3D();
    VECTOR_3D gradientNumeratorMax = VECTOR_3D();
    VECTOR_3D gradientDenominatorMax = VECTOR_3D();
    VECTOR_3D gradientNumeratorMin = VECTOR_3D();
    VECTOR_3D gradientDenominatorMin = VECTOR_3D();
    VECTOR_3D gradientMax = VECTOR_3D();
    VECTOR_3D gradientMin = VECTOR_3D();

    POS_3D curPinPosition = curPin->getAbsolutePos();

    // X dimension
    if (!curPin->expZeroFlgMax_WA.x)
    {
        gradientDenominatorMax.x = invertedGamma.x * curPin->eMax_WA.x;
        gradientNumeratorMax.x = curPin->eMax_WA.x + curPinPosition.x * gradientDenominatorMax.x;
        gradientMax.x =
            (gradientNumeratorMax.x * denominatorMax_WA.x - gradientDenominatorMax.x * numeratorMax_WA.x) /
            (denominatorMax_WA.x * denominatorMax_WA.x);
    }

    if (!curPin->expZeroFlgMin_WA.x)
    {
        gradientDenominatorMin.x = invertedGamma.x * curPin->eMin_WA.x;
        gradientNumeratorMin.x = curPin->eMin_WA.x - curPinPosition.x * gradientDenominatorMin.x;
        gradientMin.x =
            (gradientNumeratorMin.x * denominatorMin_WA.x + gradientDenominatorMin.x * numeratorMin_WA.x) /
            (denominatorMin_WA.x * denominatorMin_WA.x);
    }

    // Y dimension
    if (!curPin->expZeroFlgMax_WA.y)
    {
        gradientDenominatorMax.y = invertedGamma.y * curPin->eMax_WA.y;
        gradientNumeratorMax.y = curPin->eMax_WA.y + curPinPosition.y * gradientDenominatorMax.y;
        gradientMax.y =
            (gradientNumeratorMax.y * denominatorMax_WA.y - gradientDenominatorMax.y * numeratorMax_WA.y) /
            (denominatorMax_WA.y * denominatorMax_WA.y);
    }

    if (!curPin->expZeroFlgMin_WA.y)
    {
        gradientDenominatorMin.y = invertedGamma.y * curPin->eMin_WA.y;
        gradientNumeratorMin.y = curPin->eMin_WA.y - curPinPosition.y * gradientDenominatorMin.y;
        gradientMin.y =
            (gradientNumeratorMin.y * denominatorMin_WA.y + gradientDenominatorMin.y * numeratorMin_WA.y) /
            (denominatorMin_WA.y * denominatorMin_WA.y);
    }

    // Z dimension
    if (!curPin->expZeroFlgMax_WA.z)
    {
        gradientDenominatorMax.z = invertedGamma.z * curPin->eMax_WA.z;
        gradientNumeratorMax.z = curPin->eMax_WA.z + curPinPosition.z * gradientDenominatorMax.z;
        gradientMax.z =
            (gradientNumeratorMax.z * denominatorMax_WA.z - gradientDenominatorMax.z * numeratorMax_WA.z) /
            (denominatorMax_WA.z * denominatorMax_WA.z);
    }

    if (!curPin->expZeroFlgMin_WA.z)
    {
        gradientDenominatorMin.z = invertedGamma.z * curPin->eMin_WA.z;
        gradientNumeratorMin.z = curPin->eMin_WA.z - curPinPosition.z * gradientDenominatorMin.z;
        gradientMin.z =
            (gradientNumeratorMin.z * denominatorMin_WA.z + gradientDenominatorMin.z * numeratorMin_WA.z) /
            (denominatorMin_WA.z * denominatorMin_WA.z);
    }

    gradientOnCurrentPin.x = gradientMax.x - gradientMin.x;
    gradientOnCurrentPin.y = gradientMax.y - gradientMin.y;
    gradientOnCurrentPin.z = gradientMax.z - gradientMin.z;
    return gradientOnCurrentPin;
}

VECTOR_3D Net::getWirelengthGradientLSE_3D(VECTOR_3D invertedGamma, Pin *curPin)
{
    VECTOR_3D gradientOnCurrentPin = VECTOR_3D();
    VECTOR_3D gradientMax = VECTOR_3D();
    VECTOR_3D gradientMin = VECTOR_3D();

    gradientMax.x = (curPin->expZeroFlgMax_LSE.x ? 0 : curPin->eMax_LSE.x) / sumMax_LSE.x;
    gradientMin.x = (curPin->expZeroFlgMin_LSE.x ? 0 : curPin->eMin_LSE.x) / sumMin_LSE.x;
    gradientMax.y = (curPin->expZeroFlgMax_LSE.y ? 0 : curPin->eMax_LSE.y) / sumMax_LSE.y;
    gradientMin.y = (curPin->expZeroFlgMin_LSE.y ? 0 : curPin->eMin_LSE.y) / sumMin_LSE.y;
    gradientMax.z = (curPin->expZeroFlgMax_LSE.z ? 0 : curPin->eMax_LSE.z) / sumMax_LSE.z;
    gradientMin.z = (curPin->expZeroFlgMin_LSE.z ? 0 : curPin->eMin_LSE.z) / sumMin_LSE.z;

    gradientOnCurrentPin.x = gradientMax.x - gradientMin.x;
    gradientOnCurrentPin.y = gradientMax.y - gradientMin.y;
    gradientOnCurrentPin.z = gradientMax.z - gradientMin.z;
    return gradientOnCurrentPin;
}


VECTOR_2D Net::getP2pAttractionGradient_2D(Pin *curPin, PlaceDB* db)
{
    // Q(i,j) (x_i - x_j)^2 * w_ij
    // dQ(i,j)/dx_i = 2 * (x_i - x_j) * w_ij
    // dQ(i,j)/dx_j = -2 * (x_i - x_j) * w_ij

    assert(curPin);
    VECTOR_2D gradientOnCurrentPin;
    gradientOnCurrentPin.SetZero();

    POS_3D curPinPos = curPin->getAbsolutePos();

    for (Pin* otherPin : netPins)
    {
        if (otherPin == curPin)
        {
            continue; // skip the current pin
        }
        
        float currentPinPairWeight = db -> getP2Pweight(curPin, otherPin); // check if pin1 and pin2 exist
        // if (currentPinPairWeight != 1 ) cout <<"( "<< curPin->name <<", " << otherPin->name<<" ): " << currentPinPairWeight<<endl; 
        // assume weight is 1, can be changed later
        POS_3D otherPinPos = otherPin->getAbsolutePos();

        // weight is 1
        gradientOnCurrentPin.x += currentPinPairWeight * (curPinPos.x - otherPinPos.x);
        gradientOnCurrentPin.y += currentPinPairWeight * (curPinPos.y - otherPinPos.y);
    }
   
    return gradientOnCurrentPin;
}


POS_3D Pin::getAbsolutePos()
{
    // POS_3D absPos;
    // module->calcCenter();//?
    return absolutePos;
}

// POS_3D Pin::fetchAbsolutePos()
// {
//     return absolutePos;
// }

void Pin::calculateAbsolutePos()
{
    absolutePos.x = module->getCenter().x + offset.x;
    absolutePos.y = module->getCenter().y + offset.y;
    absolutePos.z = module->getCenter().z;
}

void Pin::setId(int index)
{
    idx = index;
}

void Pin::setNet(Net *_net)
{
    net = _net;
}

void Pin::setModule(Module *_module)
{
    module = _module;
}

void Pin::setDirection(int _direction)
{
    direction = _direction;
}

void Module::addPin(Pin *_pin)
{
    modulePins.push_back(_pin);
    nets.push_back(_pin->net);
}

int Module::getTotalConnectedPinsNum()
{
    if (totalConnectedPinsNum != -1)
    {
        return totalConnectedPinsNum; // return cached value
    }
    
    // calculate total connected pins num
    int pins_count = 0;
    for (Pin *curPin : modulePins)
    {
        if (curPin->net)
        {
            pins_count += curPin->net->getPinCount() - 1; // -1 because we don't count the current pin itself
        }
    }
    totalConnectedPinsNum = pins_count; // cache the value
    return totalConnectedPinsNum;
}

POS_2D Module::getLL_2D()
{
    POS_2D ll_2D;
    ll_2D.x = coor.x;
    ll_2D.y = coor.y;
    return ll_2D;
}

POS_2D Module::getUR_2D()
{
    POS_2D ur_2D;

    ur_2D.x = coor.x;
    ur_2D.y = coor.y;

    ur_2D.x += width;
    ur_2D.y += height;

    // assert(width != 0 && height != 0);
    return ur_2D;
}

// 3D methods implementation
POS_3D Module::getLL_3D()
{
    return coor; // coor is already 3D (lower-left-front corner)
}

POS_3D Module::getUR_3D()
{
    POS_3D ur_3D;
    ur_3D.x = coor.x + width;
    ur_3D.y = coor.y + height;
    ur_3D.z = coor.z + depth;
    return ur_3D;
}

void Module::setOrientation(int _oritentation)
{
    orientation = _oritentation;
}

//! need to check if coor is out side of the chip!!! but should be done in placeDB
void Module::setLocation_2D(float _x, float _y, float _z)
{
    coor.x = _x;
    coor.y = _y;
    coor.z = _z;
    // update center
    center.x = coor.x + (float)0.5 * width; //! be careful of float problems
    center.y = coor.y + (float)0.5 * height;
    center.z = coor.z;
}

//! need to check if coor is out side of the chip!!! but should be done in placeDB
void Module::setInitialLocation_2D(float _x, float _y, float _z)
{
    initialcoor.x = _x;
    initialcoor.y = _y;
    initialcoor.z = _z;
    // update center
    initialcenter.x = initialcoor.x + (float)0.5 * width; //! be careful of float problems
    initialcenter.y = initialcoor.y + (float)0.5 * height;
    initialcenter.z = initialcoor.z;
}


void Module::setCenter_2D(float _x, float _y, float _z)
{
    center.x = _x;
    center.y = _y;
    center.z = _z;
    // update coor
    coor.x = center.x - (float)0.5 * width; //! be careful of float problems
    coor.y = center.y - (float)0.5 * height;
    coor.z = center.z;
}

// 3D setter methods implementation
void Module::setLocation_3D(float _x, float _y, float _z)
{
    coor.x = _x;
    coor.y = _y;
    coor.z = _z;
    // update center with 3D depth consideration
    center.x = coor.x + (float)0.5 * width;
    center.y = coor.y + (float)0.5 * height;
    center.z = coor.z + (float)0.5 * depth; // Include depth for true 3D center
}

void Module::setInitialLocation_3D(float _x, float _y, float _z)
{
    initialcoor.x = _x;
    initialcoor.y = _y;
    initialcoor.z = _z;
    // update initial center with 3D depth consideration
    initialcenter.x = initialcoor.x + (float)0.5 * width;
    initialcenter.y = initialcoor.y + (float)0.5 * height;
    initialcenter.z = initialcoor.z + (float)0.5 * depth; // Include depth for true 3D center
}

void Module::setCenter_3D(float _x, float _y, float _z)
{
    center.x = _x;
    center.y = _y;
    center.z = _z;
    // update coor with 3D depth consideration
    coor.x = center.x - (float)0.5 * width;
    coor.y = center.y - (float)0.5 * height;
    coor.z = center.z - (float)0.5 * depth; // Include depth for true 3D positioning
}

POS_2D SiteRow::getLL_2D()
{
    return start;
}

POS_2D SiteRow::getUR_2D()
{
    POS_2D ur_2D = end;
    ur_2D.y += height;
    return ur_2D;
}

// 3D methods for SiteRow
POS_3D SiteRow::getLL_3D()
{
    POS_3D ll_3D;
    ll_3D.x = start.x;
    ll_3D.y = start.y;
    ll_3D.z = front;
    return ll_3D;
}

POS_3D SiteRow::getUR_3D()
{
    POS_3D ur_3D;
    ur_3D.x = end.x;
    ur_3D.y = end.y + height;
    ur_3D.z = front + depth;
    return ur_3D;
}
