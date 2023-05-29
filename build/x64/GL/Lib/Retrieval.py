import copy
import heapq
import os
import random

import joblib
import numpy as np
import warnings
import torchvision.models as models
from torchvision import transforms
from PIL import Image
import torch
import torch.nn as nn
import torch.optim as optim
import NetVlad
import h5py
from treelib import Tree


file_path = "DebugData.txt"

def ParseVocTree(VocTreePath):
    f = open(VocTreePath)
    VocTree = Tree()
    for line in f:
        NodeInfoList = line.strip().split(',')
        NodeID = NodeInfoList[0]
        NodeData = np.asarray(NodeInfoList[1].split(';')).astype(float)
        ChildrenIDs = []
        if len(NodeInfoList[2])!=0:
            ChildrenIDs = NodeInfoList[2].split(';')
        if not VocTree.contains(NodeID):
            VocTree.create_node(identifier=NodeID, data=NodeData)
        else:
            VocTree.update_node(NodeID, data=NodeData)
        for ChildID in ChildrenIDs:
            VocTree.create_node(identifier=ChildID, data=None, parent=NodeID)
    return VocTree

# 在tree中, 获取parent节点的所有孩子节点中, 距离最小的那个节点
def GetMinDist(data, tree, parent):
    ChildrenList = tree.children(parent)
    ChildrenDist = [0] * len(ChildrenList)
    for ChildIndex in range(len(ChildrenList)):
        ChildrenDist[ChildIndex] = np.linalg.norm(data - ChildrenList[ChildIndex].data)
    MinIndex = ChildrenDist.index(min(ChildrenDist))
    return ChildrenList[MinIndex].identifier


# 获取tree的最小距离路径
def GetMinDistPath(data, tree):
    MinDistPath = [tree.root]  # 树的根节点一定在最小距离路径中
    # 如果tree的根节点就是叶子节点, 那么直接返回当前的最小距离路径(只有根节点)
    if tree.get_node(tree.root).is_leaf():
        return MinDistPath
    # 开始递归
    TempParent = tree.root
    while True:
        MinDistId = GetMinDist(data, tree, TempParent)
        MinDistPath.append(MinDistId)  # 加入最小距离路径
        if tree.get_node(MinDistId).is_leaf():  # 如果已经递归到叶子节点, 返回
            return MinDistPath
        TempParent = MinDistId

# 在tree中, 获取Id号节点的所有兄弟节点的所有叶子节点
def GetBrotherLeaves(tree, Id):
    BrotherLeaves = []
    BrotherList = tree.siblings(Id)  # BrotherList为Id号节点的所有兄弟节点
    for BrotherId in BrotherList:  # 依次找每一个兄弟节点的叶子节点
        BrotherLeaves = BrotherLeaves + tree.subtree(BrotherId.identifier).leaves()
    return BrotherLeaves

class RetrievalMatchClass:
    def __init__(self, PthPath, HDF5Path, CheckPointPath, PCA_Model, VocTreePath):
        if os.path.exists(file_path):
            os.remove(file_path)
        open(file_path, 'w').close()
        with open(file_path, 'a') as file:
            file.write("0\n")
            file.close()
        random.seed()
        np.random.seed()
        torch.manual_seed(42)
        torch.cuda.manual_seed(42)
        self.num_clusters = 64
        self.encoder_dim = 512
        self.NewWidth = 224
        self.NewHeight = 224
        margin = 1
        LearnRate = 0.0001
        LearnRateStep = 5
        LearnRateGamma = 0.5
        momentum = 0.9
        weightDecay = 0.001
        self.PthPath = PthPath
        self.HDF5Path = HDF5Path
        self.CheckPointPath = CheckPointPath
        self.device = torch.device("cuda")
        self.DatabaseFeature = None
        with open(file_path, 'a') as file:
            file.write("1\n")
            file.close()
        self.pca = joblib.load(PCA_Model)
        with open(file_path, 'a') as file:
            file.write("2\n")
            file.close()

        self.ImageID2ImageName = {}
        self.VocTree = None
        self.LeafID2ImageID = {}  # 叶子ID -> 影像序号
        self.ImageNum = 0         # 影像总数
        
        with open(file_path, 'a') as file:
            file.write("3\n")
            file.close()
        if (VocTreePath is not None) and len(VocTreePath) > 0:
            self.VocTree = ParseVocTree(VocTreePath)
        with open(file_path, 'a') as file:
            file.write("4\n")
            file.close()

        warnings.filterwarnings("ignore", category=UserWarning)
        self.encoder = models.vgg16(pretrained=False)
        self.encoder.load_state_dict(torch.load(self.PthPath))
        layers = list(self.encoder.features.children())[:-2]
        with open(file_path, 'a') as file:
            file.write("5\n")
            file.close()
        self.encoder = nn.Sequential(*layers)
        self.model = nn.Module()
        self.model.add_module('encoder', self.encoder)
        with open(file_path, 'a') as file:
            file.write("6\n")
            file.close()
        net_vlad = NetVlad.NetVLAD(num_clusters=self.num_clusters, dim=self.encoder_dim, vladv2=False)
        with open(file_path, 'a') as file:
            file.write("7\n")
            file.close()
        with h5py.File(self.HDF5Path, mode='r') as h5:
            clsts = h5.get("centroids")[...]
            traindescs = h5.get("descriptors")[...]
            net_vlad.init_params(clsts, traindescs)
            del clsts, traindescs
        with open(file_path, 'a') as file:
            file.write("8\n")
            file.close()
        self.model.add_module('pool', net_vlad)
        self.model.encoder = nn.DataParallel(self.model.encoder)
        self.model.pool = nn.DataParallel(self.model.pool)
        with open(file_path, 'a') as file:
            file.write("9\n")
            file.close()
        optimizer = optim.SGD(filter(lambda p: p.requires_grad, self.model.parameters()), lr=LearnRate, momentum=momentum, weight_decay=weightDecay)
        scheduler = optim.lr_scheduler.StepLR(optimizer, step_size=LearnRateStep, gamma=LearnRateGamma)
        criterion = nn.TripletMarginLoss(margin=margin, p=2, reduction='sum').to(self.device)
        with open(file_path, 'a') as file:
            file.write("10\n")
            file.close()
        checkpoint = torch.load(self.CheckPointPath, map_location=lambda storage, loc: storage)
        TestEpoch = checkpoint['epoch']
        BestMetric = checkpoint['best_score']
        self.model.load_state_dict(checkpoint['state_dict'], strict=False)
        self.model = self.model.to(self.device)
        with open(file_path, 'a') as file:
            file.write("11\n")
            file.close()
    def Match(self, ImagePath, N):
        with open(file_path, 'a') as file:
            file.write(ImagePath+"##"+str(N)+"\n")
            file.write("12\n")
            file.close()            
        ImagePath = ImagePath.replace("\\", "/")
        image = Image.open(ImagePath)
        with open(file_path, 'a') as file:
            file.write("13\n")
            file.close()
        if len(image.split()) != 3:
            image = image.convert('RGB')
        with open(file_path, 'a') as file:
            file.write("14\n")
            file.close()            
        image = image.resize((self.NewWidth, self.NewHeight))
        with open(file_path, 'a') as file:
            file.write("15\n")
            file.close()
        image = transforms.Compose([transforms.ToTensor()])(image).to(self.device)
        with open(file_path, 'a') as file:
            file.write("16\n")
            file.close()
        self.model.eval()
        with torch.no_grad():
            with open(file_path, 'a') as file:
                file.write("17\n")
                file.close()
            Feature = self.model.encoder(image) # 经过VGG16
            with open(file_path, 'a') as file:
                file.write("18\n")
                file.close()
            Feature = Feature.unsqueeze(0)
            with open(file_path, 'a') as file:
                file.write("19\n")
                file.close()
            PoolingFeature = self.model.pool(Feature)  # 经过NetVlad
            with open(file_path, 'a') as file:
                file.write("20\n")
                file.close()
            PoolingFeature = self.pca.transform(PoolingFeature.to(torch.device("cpu")))  # tensor转numpy, 再做PCA降维
            with open(file_path, 'a') as file:
                file.write("21\n")
                file.close()
            CurrentImageID = self.ImageNum # 当前影像的序号
            self.ImageID2ImageName[CurrentImageID] = os.path.basename(ImagePath)
            self.ImageNum = self.ImageNum + 1
            with open(file_path, 'a') as file:
                file.write("22\n")
                file.close()
            if self.VocTree is None:       # 用两两匹配
                with open(file_path, 'a') as file:
                    file.write("23\n")
                    file.close()
                PoolingFeature = torch.from_numpy(PoolingFeature).to(self.device)
                with open(file_path, 'a') as file:
                    file.write("24\n")
                    file.close()
                TopNIndex = []
                if self.DatabaseFeature is not None:
                    with open(file_path, 'a') as file:
                        file.write("25\n")
                        file.close()
                    Database = self.DatabaseFeature.to(self.device)
                    QueryFeat2 = torch.sum(torch.mul(PoolingFeature, PoolingFeature), dim=1)
                    DbImageFeat2 = torch.sum(torch.mul(Database, Database), dim=1)
                    Temp1 = torch.reshape(QueryFeat2, (-1, 1)).repeat(1, Database.shape[0])
                    Temp2 = 2 * torch.mm(PoolingFeature, Database.transpose(0, 1))
                    Temp3 = DbImageFeat2.repeat(1, 1)
                    with open(file_path, 'a') as file:
                        file.write("26\n")
                        file.close()
                    QueryDB_Distance2 = torch.abs(Temp1 - Temp2 + Temp3).to(torch.device("cpu")).numpy()[0]
                    TopNIndex = heapq.nsmallest(N, range(len(QueryDB_Distance2)), QueryDB_Distance2.__getitem__)
                    with open(file_path, 'a') as file:
                        file.write("27\n")
                        file.close()
                ThisImageFeature = PoolingFeature
                if self.DatabaseFeature is None:
                    self.DatabaseFeature = ThisImageFeature.to(torch.device("cpu"))
                else:
                    self.DatabaseFeature = torch.cat((self.DatabaseFeature, ThisImageFeature.to(torch.device("cpu"))), 0)
                with open(file_path, 'a') as file:
                    file.write("28\n")
                    file.close()
                return TopNIndex
            else:  # 用词汇树做匹配
                with open(file_path, 'a') as file:
                    file.write("29\n")
                    file.close()
                if self.DatabaseFeature is None:   #当前特征加入数据库
                    self.DatabaseFeature = [PoolingFeature.squeeze()]
                else:
                    self.DatabaseFeature.extend(PoolingFeature)
                with open(file_path, 'a') as file:
                    file.write("30\n")
                    file.close()                    
                MinDistPath = GetMinDistPath(PoolingFeature, self.VocTree)[::-1] #计算关键路径
                TopNIndex = []
                with open(file_path, 'a') as file:
                    file.write("31\n")
                    file.close()
                if MinDistPath[0] not in self.LeafID2ImageID:  #影像所属的叶子节点还没有任何影像
                    self.LeafID2ImageID[MinDistPath[0]] = [CurrentImageID]  #把当前影像ID加入该叶子节点中
                    with open(file_path, 'a') as file:
                        file.write("32\n")
                        file.close()
                else:
                    with open(file_path, 'a') as file:
                        file.write("33\n")
                        file.close()
                    TopNIndex = copy.deepcopy(self.LeafID2ImageID[MinDistPath[0]])  #取出影像所属的叶子节点下的当前所有影像ID(不包含当前影像的ID)
                    self.LeafID2ImageID[MinDistPath[0]].append(CurrentImageID) #再把当前影像ID加入该叶子节点
                if len(TopNIndex) < N: # 叶子节点的影像太少
                    with open(file_path, 'a') as file:
                        file.write("34\n")
                        file.close()
                    for BrotherNode in self.VocTree.siblings(MinDistPath[0]): #找所有的兄弟节点
                        BrotherID = BrotherNode.identifier
                        if BrotherID in self.LeafID2ImageID:
                            TopNIndex.extend(copy.deepcopy(self.LeafID2ImageID[BrotherID])) #把所有兄弟节点的所有影像ID加入
                    for i in range(1,len(MinDistPath)): #从叶子节点的父节点开始往上遍历关键路径(直到根节点), MinDistPath[i](i>0)全为非叶子节点
                        if len(TopNIndex) >=N:  #影像数已经够了, 不再继续遍历
                            break
                        MoreLeavesID = GetBrotherLeaves(self.VocTree, MinDistPath[i]) #找关键路径的当前非叶子节点的所有兄弟节点的叶子节点
                        for NodeID in MoreLeavesID:
                            if NodeID.identifier in self.LeafID2ImageID:
                                TopNIndex.extend(copy.deepcopy(self.LeafID2ImageID[NodeID.identifier])) #将关键路径的当前非叶子节点的所有兄弟节点的叶子节点中的影像ID加入
                    with open(file_path, 'a') as file:
                        file.write("35\n")
                        file.close()
                AllDistance = [0] * len(TopNIndex) #计算得到的所有影像与当前影像的距离
                with open(file_path, 'a') as file:
                    file.write("36\n")
                    file.close()
                for i in range(len(TopNIndex)):
                    AllDistance[i] = np.linalg.norm(PoolingFeature - self.DatabaseFeature[TopNIndex[i]])
                with open(file_path, 'a') as file:
                    file.write("37\n")
                    file.close()
                TopNIndex_Index = heapq.nsmallest(N, range(len(AllDistance)), AllDistance.__getitem__)   # 按照距离排序取前N个, TopNIndex_Index是TopNIndex的索引(索引的索引)
                TopNIndex_Sorted = [0] * len(TopNIndex_Index)
                for i in range(len(TopNIndex_Index)): # TopNIndex_Index[0]对应的TopNIndex距离最小, TopNIndex_Index[1]对应的TopNIndex距离第二小...
                    TopNIndex_Sorted[i] = TopNIndex[TopNIndex_Index[i]]
                with open(file_path, 'a') as file:
                    file.write("38\n")
                    file.close()
                with open(file_path, 'a') as file:
                    file.write("####" + str(CurrentImageID) + "####" + str(len(TopNIndex_Sorted)) + "\n")
                    for item in TopNIndex_Sorted:
                        file.write(str(item) + "\t")
                    file.write("\n")
                    file.close()
                return TopNIndex_Sorted
    def OutputInfo(self, Path):
        if self.VocTree is not None:
            f = open(Path, "w")
            for LeafID, ImageIDs in self.LeafID2ImageID.items():
                f.write(LeafID + ": ")
                for i in range(len(ImageIDs)):
                    f.write(self.ImageID2ImageName[ImageIDs[i]])
                    if i!=len(ImageIDs)-1:
                        f.write(",")
                f.write("\n")