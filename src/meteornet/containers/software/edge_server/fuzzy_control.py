class FuzzyControl:


    def __init__(self, input_partitions=[1.0/8, 2*1.0/8, 3*1.0/8, 4*1.0/8, 5*1.0/8, 6*1.0/8, 7*1.0/8],
                 output_partition=[1.0/8, 2*1.0/8, 3*1.0/8, 4*1.0/8, 5*1.0/8, 6*1.0/8, 7*1.0/8]):
        self.partitions = input_partitions
        self.output_partition = output_partition
        # self.n_partitions = n_partitions


    # Function for triangular fuzzyfication  
    def triangular(self,x, a, b, c):
        return max(min((x-a)/(b-a), (c-x)/(c-b)),0)
    
    def open_left(self, x, alpha, beta):
        if x<=alpha:
            return 1
        if alpha<x and x<=beta:
            return (beta - x)/(beta - alpha)
        else:
            return 0
    
    def open_right(self, x, alpha, beta):
        if x<=alpha:
            return 0
        if alpha<x and x<=beta:
            return (x-alpha)/(beta - alpha)
        else:
            return 1
        
    def area_triangle(self, mu, a,b,c):
        x1 = mu*(b-a) + a
        x2 = c - mu*(c-b)
        d1 = (c-a); d2 = x2-x1
        a = (1/2)*mu*(d1 + d2)
        return a, b

    def area_open_left(self, mu, alpha, beta):
        xOL = beta -mu*(beta - alpha)
        return 1/2*mu*(beta+ xOL), beta/2

    def area_open_right(self, mu, alpha, beta):
        xOR = (beta - alpha)*mu + alpha
        aOR = (1/2)*mu*((1.0 - alpha) + (1.0 -xOR))
        return aOR, (1.0 - alpha)/2 + alpha
    
    def compare(self, TC1, TC2):
        TC = 0
        if TC1>TC2 and TC1 !=0 and TC2 !=0:
            TC = TC2
        else:
            TC = TC1
        
        if TC1 == 0 and TC2 !=0:
            TC = TC2
            
        if TC2 == 0 and TC1 !=0:
            TC = TC1
            
        return TC
    

    # R1: if TF is high  then S is on
    # R2: if TF is mid  then S is zero
    # R3: if TF is low and TR is high then S is zero
    # R4: if TF is low and TR is low then S is off
    def basic_rules(self, input_fuzzy):
        output_fuzzy = [None]*3

        # R1
        output_fuzzy[2] = input_fuzzy[0][2]
        # R2
        r2 = input_fuzzy[0][1]
        # R3
        r3 = min(input_fuzzy[0][0], input_fuzzy[1][1])
        output_fuzzy[1] = self.compare(r2, r3)
        # R4
        output_fuzzy[0] = min(input_fuzzy[0][0], input_fuzzy[1][0])
        return output_fuzzy
    
    # R1: if TF is high                              then S is on
    # R2: if TF is mid and TO is mid                 then S is on

    # R3: if PC is low and TO is mid                 then S is zero
    # R4: if TF is low and PC is high                then S is zero

    # R5: if PC is mid and CU is mid and TF is low   then S is off
    # R6: if TO is low and PC is low and TF is low   then S is off
    # input -> [TF, TO, PC, CU]

    # tf_mess = 0.0
    # to_mess = 0.0
    # c_mess = 30
    def rules(self, input_fuzzy):
        output_fuzzy = [None]*3

        TF = 0
        TO = 1
        PC = 2
        CU = 3

        HI = 2
        MI = 1
        LO = 0

        # R1
        r1 = input_fuzzy[TF][HI]
        # R2
        r2 = min(input_fuzzy[TF][MI], input_fuzzy[TO][MI])

        output_fuzzy[2] = self.compare(r1, r2)
        # output_fuzzy[2] = self.compare(r3, output_fuzzy[2])
        
        # R3
        r3 = min(input_fuzzy[PC][LO], input_fuzzy[TO][MI])
        # R4
        r4 = min(input_fuzzy[TF][LO], input_fuzzy[PC][HI])
        output_fuzzy[1] = self.compare(r3, r4)

        # R5
        r5 = min(input_fuzzy[PC][MI], input_fuzzy[CU][MI], input_fuzzy[TF][LO])
        # R6
        r6 = min(input_fuzzy[TO][LO], input_fuzzy[PC][LO], input_fuzzy[TF][LO])
        output_fuzzy[0] = self.compare(r5, r6)
        # output_fuzzy[0] = r6

        return output_fuzzy
        
    def calculate_partition(self, x):

        mu_x_vector = []
        for n_x in range(len(self.partitions)):
            partitions = self.partitions[n_x]
            if len(partitions) < 4:
                return None
            elif (len(partitions) - 4) % 3 == 0:
                n_sets = int(2 + (len(partitions) - 4)/3)
                mu_x = [None]*n_sets
                mu_x[0] = self.open_left(x[n_x], partitions[0], partitions[1])
                mu_x[-1] = self.open_right(x[n_x], partitions[-2], partitions[-1])
                for i in range(1, n_sets-1):
                    mu_x[i] = self.triangular(x[n_x], *partitions[2+(i-1)*3: 2+(i-1)*3+3])
                mu_x_vector.append(mu_x)
            else:
                return None
        return mu_x_vector

    def plot_mu(self, x):

        fig, ax = plt.subplots(1, 1)

        ax.plot([0, self.partitions[0], self.partitions[1]],[1, 1, 0], label='Error Low')

        for i in range(1, self.n_partitions-1):
            ax.plot(
                [self.partitions[2+(i-1)*3],  self.partitions[3+(i-1)*3], self.partitions[4+(i-1)*3]],
                [0, 1, 0], label='Error Medium')
        

        ax.plot([self.partitions[-2], self.partitions[-1], 1],[0, 1, 1], label='Error High')
        ax.set_ylim(0,1.1)
        ax.set_xlim(0,1.1)
        ax.set_xlabel('$e_{gs}$', size=30)
        ax.set_ylabel('$ \mu_{P}(e_{gs})$',size=30)
        ax.set_title('$ \mu_{P}(e_{gs})$  VS $e_{gs}$', size=30)
        ax.tick_params(which="major", labelsize=30)
        ax.legend(fontsize=20)
        plt.show()


        fig, ax = plt.subplots(1, 1)

        ax.plot([0, self.output_partition[0], self.output_partition[1]],[1, 1, 0], label='Server Off')

        for i in range(1, self.n_partitions-1):
            ax.plot(
                [self.output_partition[2+(i-1)*3],  self.output_partition[3+(i-1)*3], self.output_partition[4+(i-1)*3]],
                [0, 1, 0], label='Server Zero')
        

        ax.plot([self.output_partition[-2], self.output_partition[-1], 1],[0, 1, 1], label='Server On')
        ax.set_ylim(0,1.1)
        ax.set_xlim(0,1.1)
        ax.set_xlabel('$S_{g}$', size=30)
        ax.set_ylabel('$ \mu_{Q}(S_{g})$', size=30)
        ax.set_title('$ \mu_{Q}(S_{g})$  VS $S_{g}$', size=30)
        ax.tick_params(which="major", labelsize=30)
        ax.legend(fontsize=20)
        plt.show()


        

    def defuzzyfication(self, output):

        if len(self.output_partition) < 4 or (len(self.output_partition) - 4) % 3 != 0:
            return None

        out_sets = int(2 + (len(self.output_partition) - 4)/3)

        areas = [0] * out_sets
        centers = [0] * out_sets
        
        for i in range(3):
            if i==0 and output[0] !=0:
                areas[0], centers[0] = self.area_open_left(output[i], *self.output_partition[:2])
            elif i == out_sets-1 and output[i] !=0:
                areas[i], centers[i] = self.area_open_right(output[i], *self.output_partition[5:7])
            elif output[i] !=0:
                areas[i], centers[i] = self.area_triangle(output[i], *self.output_partition[2:5])

            
        numerator = sum([a*c for a, c in zip(areas, centers)])
        denominator = sum(areas)
        if denominator ==0:
            print("No rules exist to give the result")
            return None
        else:
            crispOutput = numerator/denominator
            return crispOutput


    def predict(self, x):
        fuzzy_inputs = self.calculate_partition(x)
        fuzzy_outputs = self.rules(fuzzy_inputs)
        print('fuzzy outputs {}'.format(fuzzy_outputs))
        crisp_output_final = self.defuzzyfication(fuzzy_outputs)
        return crisp_output_final



if __name__ == '__main__':

    import matplotlib.pyplot as plt
    fc = FuzzyControl(input_partitions=[[0.03, 0.1, 0.05, 0.1, 0.45, 0.15, 0.6], 
                                        [0.1, 0.2, 0.1, 0.2],
                                        [5, 10, 5, 10, 25, 25, 60],
                                        [0.05, 0.1, 0.05, 0.1]],
                       output_partition=[0.25, 0.375, 0.25, 0.5, 0.75, 0.625, 0.75])


    # fc = FuzzyControl(input_partitions=[[0.1, 0.3, 0.05, 0.25, 0.45, 0.15, 0.6], 
    #                                     [0.1, 0.2, 0.1, 0.2],
    #                                     [20, 30, 20, 30]],
    #                    output_partition=[0.25, 0.375, 0.25, 0.5, 0.75, 0.625, 0.75])

    tf_mess = 0.1
    to_mess = 0.0
    c_mess  = 24
    cu_mess  = 0.2
    x_mess = [tf_mess, to_mess, c_mess, cu_mess]
    print('x mess: {}'.format(x_mess))

    print(fc.calculate_partition(x_mess))

    print(fc.predict(x_mess))



    # e_mess = [i /10 for i in range(10)]

    # y = [fc.predict(e) for e in e_mess]

    # plt.plot(e_mess, y)
    # plt.show()
    # fc.plot_mu(0.1)



    # mu_e1 = fc.triangular(e_mess, 0.1, 0.35, 0.6)
    # print(mu_e1)

    # print(fc.open_left(e_mess, 0.2, 0.2))
    # print(fc.open_right(e_mess, 0.5, 0.6))
    




