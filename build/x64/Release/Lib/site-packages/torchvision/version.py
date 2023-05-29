__version__ = '0.14.1+cu117'
git_version = '5e8e2f125f140d1e908cf424a6a85cacad758125'
from torchvision.extension import _check_cuda_version
if _check_cuda_version() > 0:
    cuda = _check_cuda_version()
